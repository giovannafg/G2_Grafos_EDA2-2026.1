const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const { spawn, spawnSync } = require('node:child_process');

const root = __dirname;
const preferredPort = Number(process.env.PORT || 3000);
const engineExe = path.join(root, process.platform === 'win32' ? 'campo_engine.exe' : 'campo_engine');

function compileEngine() {
  const sources = ['campo_engine.c', 'bomba.c', 'bomba.h'].map((file) => path.join(root, file));
  const engineExists = fs.existsSync(engineExe);
  const engineMtime = engineExists ? fs.statSync(engineExe).mtimeMs : 0;
  const needsCompile = !engineExists || sources.some((file) => fs.statSync(file).mtimeMs > engineMtime);

  if (!needsCompile) return;

  const result = spawnSync('gcc', ['campo_engine.c', 'bomba.c', '-o', engineExe], {
    cwd: root,
    stdio: 'inherit',
  });

  if (result.status !== 0) {
    throw new Error('Nao consegui compilar campo_engine.c com gcc.');
  }
}

compileEngine();

const engine = spawn(engineExe, [], {
  cwd: root,
  stdio: ['pipe', 'pipe', 'inherit'],
});

let buffer = '';
const pending = [];
let currentState = null;

engine.stdout.setEncoding('utf8');
engine.stdout.on('data', (chunk) => {
  buffer += chunk;
  const lines = buffer.split(/\r?\n/);
  buffer = lines.pop() || '';

  for (const line of lines) {
    if (!line.trim()) continue;

    let payload;
    try {
      payload = JSON.parse(line);
    } catch (error) {
      payload = { error: 'Resposta invalida do motor C.', raw: line };
    }

    if (!payload.error && payload.type !== 'hint') {
      currentState = payload;
    }
    
    const resolve = pending.shift();
    if (resolve) resolve(payload);
  }
});

engine.on('exit', (code) => {
  while (pending.length > 0) {
    const resolve = pending.shift();
    resolve({ error: `Motor C encerrou com codigo ${code}.` });
  }
});

function sendCommand(command) {
  return new Promise((resolve) => {
    pending.push(resolve);
    engine.stdin.write(`${command}\n`);
  });
}

function readJson(req) {
  return new Promise((resolve, reject) => {
    let body = '';
    req.on('data', (chunk) => {
      body += chunk;
      if (body.length > 10000) {
        req.destroy();
        reject(new Error('Corpo muito grande.'));
      }
    });
    req.on('end', () => {
      try {
        resolve(body ? JSON.parse(body) : {});
      } catch (error) {
        reject(new Error('JSON invalido.'));
      }
    });
    req.on('error', reject);
  });
}

function sendJson(res, status, payload) {
  res.writeHead(status, {
    'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store',
  });
  res.end(JSON.stringify(payload));
}

function sendFile(res, filePath, contentType) {
  fs.readFile(filePath, (error, data) => {
    if (error) {
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
      res.end('Arquivo nao encontrado.');
      return;
    }

    res.writeHead(200, {
      'Content-Type': contentType,
      'Cache-Control': 'no-store',
    });
    res.end(data);
  });
}

const server = http.createServer(async (req, res) => {
  try {
    const url = new URL(req.url, `http://${req.headers.host}`);

    if (req.method === 'GET' && (url.pathname === '/' || url.pathname === '/minado.html')) {
      sendFile(res, path.join(root, 'minado.html'), 'text/html; charset=utf-8');
      return;
    }

    if (req.method === 'GET' && url.pathname === '/api/state') {
      if (currentState) {
        sendJson(res, 200, currentState);
      } else {
        sendJson(res, 200, await sendCommand('STATE'));
      }
      return;
    }

    if (req.method === 'POST' && url.pathname === '/api/start') {
      const body = await readJson(req);
      sendJson(res, 200, await sendCommand(`START ${Number(body.level || 1)}`));
      return;
    }

    if (req.method === 'POST' && url.pathname === '/api/open') {
      const body = await readJson(req);
      sendJson(res, 200, await sendCommand(`OPEN ${Number(body.row)} ${Number(body.col)}`));
      return;
    }

    if (req.method === 'POST' && url.pathname === '/api/flag') {
      const body = await readJson(req);
      sendJson(res, 200, await sendCommand(`FLAG ${Number(body.row)} ${Number(body.col)}`));
      return;
    }

    if (req.method === 'POST' && url.pathname === '/api/hint') {
      const body = await readJson(req);
      sendJson(res, 200, await sendCommand(`HINT ${Number(body.row)} ${Number(body.col)}`));
      return;
    }

    res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('Rota nao encontrada.');
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
});

function listen(port, attemptsLeft = 10) {
  server.once('error', (error) => {
    if (error.code === 'EADDRINUSE' && attemptsLeft > 0 && !process.env.PORT) {
      console.log(`Porta ${port} ocupada. Tentando ${port + 1}...`);
      listen(port + 1, attemptsLeft - 1);
      return;
    }

    engine.stdin.write('QUIT\n');
    engine.kill();
    throw error;
  });

  server.listen(port, () => {
    console.log(`Campo Minado local: http://localhost:${port}`);
  });
}

listen(preferredPort);

process.on('SIGINT', () => {
  engine.stdin.write('QUIT\n');
  engine.kill();
  server.close(() => process.exit(0));
});