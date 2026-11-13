# arbitrage-betting

Finds the same prediction market listed on both Polymarket and Kalshi, so you can compare their prices.

The service polls open markets that close within the next 7 days from both platforms. It embeds each market question with the `bge-small-en-v1.5` (quantized) model and stores everything in Oracle. Any market whose question has cosine similarity of at least 0.95 to a market on the other platform is logged with both sides' outcomes and prices.

## How it works

- `PolymarketHandler` and `KalshiMarketHandler` run every 2 seconds (fixed delay). Each splits the next 7 days into 30 windows and pages through them in parallel.
- `ApiService` rate-limits requests per platform: one every 10 ms for Polymarket and one every 500 ms for Kalshi.
- Questions are tokenized with a WordPiece tokenizer and embedded with ONNX Runtime using CLS pooling and L2 normalization.
- Markets are upserted into the `market` table, with the embedding stored as a BLOB of big-endian floats. Platforms go into the `platform` table. Both tables are created on first start if they don't exist.

## Requirements

- CMake 3.24+
- A C++20 compiler (MSVC 2022+, GCC 13+, or Clang 17+)
- Oracle Client libraries available at runtime (Instant Client or a full Oracle home on `PATH` / `LD_LIBRARY_PATH`)
- A reachable Oracle database

The following are downloaded automatically at configure time: nlohmann/json, libcurl (unless a system one is found), ODPI-C, ONNX Runtime, and the embedding model with its tokenizer.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable, `application.properties`, the ONNX Runtime libraries and the `models` folder are all placed in the same output directory.

## Configuration

On the first configure, `config/application.properties.example` is copied to `config/application.properties`, which is gitignored. Put your credentials in that copy, or pass the path of another properties file as the first argument:

| Key | Default | Description |
| --- | --- | --- |
| `datasource.host` | `localhost` | Oracle host |
| `datasource.port` | `1521` | Oracle port |
| `datasource.sid` | `orcl` | Oracle SID |
| `datasource.username` | | Database user |
| `datasource.password` | | Database password |
| `datasource.pool-size` | `50` | Maximum pooled sessions |
| `embedding.model` | `models/bge-small-en-v1.5-q.onnx` | ONNX model path |
| `embedding.tokenizer` | `models/bge-small-en-v1.5-tokenizer.json` | Tokenizer path |
| `scheduler.fixed-delay-ms` | `2000` | Delay between polling cycles |

Relative paths are resolved against the directory of the properties file.

## Run

```sh
cd build/Release
./arbitrage_betting
```
