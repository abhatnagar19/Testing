# Nifty Lens

A single-page application for tracking the **NIFTY 50 index, its 50 constituents, or any
NSE/BSE stock** — spot price plus a futures/options view — without making you read an
option chain.

## Run it

**Recommended — official NSE data** (Node 18+, zero dependencies):

```bash
node server.js        # open http://localhost:8080
```

`server.js` serves the app and proxies NSE's official API (`nseindia.com`), handling
the cookie handshake and headers that block direct browser calls. You get the real
index/constituent quotes, real intraday charts, and the **actual option chain** —
real max pain, put/call ratio, implied volatility, ATM straddle and futures basis.

**Or just open `index.html`** in any browser — no server at all. The app then uses
Yahoo Finance for live prices (derivatives modeled), or a clearly-badged simulation
if nothing is reachable.

## The idea

Derivatives data is noisy: strikes, premiums, open interest, Greeks. Instead of showing
all of that up front, the app distills it into one answer a person actually wants:

> **Where is this likely to close today?**

Every instrument gets an **Expected close** — a single number with a ± likely range
(≈68% confidence) and a plain-language stance ("likely to close higher from here",
"flat close likely"). The complexity stays available, but behind a **Deep dive** panel
for anyone who wants to verify the reasoning.

## Features

- **Hero view** — live price, day change, prev close / open / range / VWAP for the
  selected instrument (NIFTY 50 by default).
- **Expected close card** — the prediction, its likely range, and the stance chip.
- **Intraday chart** — 5-minute price line with the projected path to 15:30 IST and an
  uncertainty wedge; crosshair + tooltip on hover; light/dark theme aware.
- **Constituents table** — all 50 Nifty stocks with price, change, trend sparkline and
  each stock's own expected close. Filter, sort, click to track.
- **Any NSE/BSE symbol** — type a symbol (e.g. `TATAPOWER`, `SENSEX`) and press Enter;
  pick the exchange from the dropdown.
- **Deep dive** (collapsed by default) — futures basis vs cost-of-carry fair value,
  max pain strike, put/call ratio, ATM implied volatility, straddle-implied move, and a
  signed breakdown of exactly what moved the expected close.

## How the expected close is computed

Starting from the current price, four measured pulls are added:

| Component | Intuition |
|---|---|
| **Momentum** | Least-squares slope of the last hour, projected to the close, damped |
| **VWAP pull** | Prices tend to mean-revert toward the volume-weighted average price |
| **Options gravity** | Heavy open interest makes the max-pain strike act like a magnet, strongest near expiry (capped at 0.6σ) |
| **Futures signal** | Futures trading above cost-of-carry fair value = traders paying up to be long |

The ± range is the one-day move implied by ATM option volatility, scaled by
√(fraction of the session remaining), so it tightens as the close approaches. After
hours, the prediction rolls to the next session.

## Data sources — honest labeling

The app tries sources in order and badges whichever it is using:

1. **Live · NSE official** — when served through `server.js`: one snapshot call gets
   the index plus all 50 constituent quotes; intraday charts, the real option chain
   (`option-chain-indices` / `option-chain-equities`) and the near futures contract
   come from the same official API. Deep-dive tiles are tagged `live · NSE`.
   BSE symbols (e.g. SENSEX) fall back to Yahoo per instrument.
2. **Live · Yahoo Finance** — when opened as a plain file: spot prices via
   `^NSEI`, `SYMBOL.NS` / `SYMBOL.BO`, fetched through public CORS proxies.
   Derivatives are modeled from live prices and tagged accordingly.
3. **Demo data (simulated)** — if nothing is reachable: a deterministic, seeded
   intraday simulation, clearly badged with a one-click retry.

## Disclaimer

Not investment advice. The expected close is a statistical estimate; markets routinely
move outside the shown range. The Nifty 50 constituent list is a static snapshot and
changes with periodic index rebalances — edit the `NIFTY50` array in `index.html` to
update it.
