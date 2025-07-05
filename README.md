# ♟️ Caro AI 15x15 – Human vs Machine

Một trò chơi Caro (Gomoku) 15x15 có thể chơi giữa người và máy (AI), hỗ trợ:
- Minimax + Alpha-Beta Pruning
- Đa luồng (multi-threading) tối ưu nước đi AI
- Chạy ở giao diện terminal (console) hoặc web HTML (WebAssembly/emsdk)
- Hỗ trợ chuyển đổi C++ → Python tương đương

---

## 📦 Cấu trúc dự án

caro_game/
├── caro.cpp # Logic AI viết bằng C++ (Minimax + Alpha-Beta + Thread)
├── run.bat # Chạy để chơi game
└── README.md # File hướng dẫn

---

## 🚀 Chạy game ở Console

### 🔧 Với C++
1. Biên dịch:
   ```bash
   run.bat
