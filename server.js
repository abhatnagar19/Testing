#!/usr/bin/env node
/* Nifty Lens local server.
   Serves index.html and proxies the official NSE (nseindia.com) JSON API,
   handling the cookie handshake and bot-protection headers that block
   direct browser calls. Zero dependencies; Node 18+.

     node server.js          # http://localhost:8080
     PORT=3000 node server.js
*/
"use strict";

const http = require("http");
const fs = require("fs");
const path = require("path");

const PORT = Number(process.env.PORT) || 8080;
const NSE = "https://www.nseindia.com";
const UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";

// Only the endpoints the app needs are forwarded.
const ALLOWED = [
  "equity-stockIndices",
  "allIndices",
  "chart-databyindex",
  "option-chain-indices",
  "option-chain-equities",
  "quote-derivative",
  "quote-equity",
  "marketStatus",
];

/* ---- NSE session (cookie) management ---- */
let cookieJar = "";
let cookieAt = 0;

async function refreshCookies() {
  const res = await fetch(NSE + "/", {
    headers: { "User-Agent": UA, "Accept": "text/html", "Accept-Language": "en-US,en;q=0.9" },
    redirect: "follow",
    signal: AbortSignal.timeout(10000),
  });
  const setCookies = typeof res.headers.getSetCookie === "function"
    ? res.headers.getSetCookie()
    : (res.headers.get("set-cookie") ? [res.headers.get("set-cookie")] : []);
  if (setCookies.length) {
    cookieJar = setCookies.map(c => c.split(";")[0]).join("; ");
    cookieAt = Date.now();
  }
  await res.arrayBuffer().catch(() => {});
  return cookieJar !== "";
}

async function nseFetch(apiPath, attempt = 0) {
  if (!cookieJar || Date.now() - cookieAt > 5 * 60 * 1000) {
    await refreshCookies().catch(() => {});
  }
  const res = await fetch(NSE + "/api/" + apiPath, {
    headers: {
      "User-Agent": UA,
      "Accept": "application/json, text/plain, */*",
      "Accept-Language": "en-US,en;q=0.9",
      "Referer": NSE + "/",
      ...(cookieJar ? { "Cookie": cookieJar } : {}),
    },
    signal: AbortSignal.timeout(12000),
  });
  if ((res.status === 401 || res.status === 403) && attempt === 0) {
    cookieJar = "";
    await refreshCookies().catch(() => {});
    return nseFetch(apiPath, 1);
  }
  if (!res.ok) throw new Error("NSE responded " + res.status);
  const text = await res.text();
  JSON.parse(text); // reject HTML error pages
  return text;
}

/* ---- tiny response cache (be polite to NSE) ---- */
const cache = new Map();
const CACHE_TTL = 15000;

async function nseCached(apiPath) {
  const hit = cache.get(apiPath);
  if (hit && Date.now() - hit.t < CACHE_TTL) return hit.body;
  const body = await nseFetch(apiPath);
  cache.set(apiPath, { t: Date.now(), body });
  if (cache.size > 300) {
    for (const [k, v] of cache) if (Date.now() - v.t > CACHE_TTL) cache.delete(k);
  }
  return body;
}

/* ---- HTTP server ---- */
const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, "http://localhost");
  res.setHeader("Access-Control-Allow-Origin", "*");

  if (url.pathname === "/api/health") {
    res.writeHead(200, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ ok: true, upstream: "nseindia.com" }));
    return;
  }

  if (url.pathname.startsWith("/api/nse/")) {
    const apiPath = url.pathname.slice("/api/nse/".length) + url.search;
    const base = apiPath.split("?")[0];
    if (!ALLOWED.includes(base)) {
      res.writeHead(403, { "Content-Type": "application/json" });
      res.end(JSON.stringify({ error: "endpoint not allowed", base }));
      return;
    }
    try {
      const body = await nseCached(apiPath);
      res.writeHead(200, { "Content-Type": "application/json", "Cache-Control": "no-store" });
      res.end(body);
    } catch (e) {
      res.writeHead(502, { "Content-Type": "application/json" });
      res.end(JSON.stringify({ error: "NSE upstream unreachable", detail: String(e.message || e) }));
    }
    return;
  }

  if (url.pathname === "/" || url.pathname === "/index.html") {
    fs.readFile(path.join(__dirname, "index.html"), (err, buf) => {
      if (err) { res.writeHead(500); res.end("index.html not found"); return; }
      res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
      res.end(buf);
    });
    return;
  }

  res.writeHead(404, { "Content-Type": "text/plain" });
  res.end("not found");
});

server.listen(PORT, () => {
  console.log(`Nifty Lens running at http://localhost:${PORT}`);
  console.log("Proxying official NSE data from nseindia.com (cookies handled automatically).");
});
