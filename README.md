# Student Expense Manager

A CLI-based personal finance tracker with a local AI advisor powered by
[llama.cpp](https://github.com/ggerganov/llama.cpp) running Mistral-7B-Instruct.

## Features

- Add, view, and delete expense records (stored in `data/expenses.csv`)
- Category-wise totals and monthly budget summary
- Overspend alerts and date-range filtering
- Export plain-text reports to `reports/`
- On-device AI financial advice via Mistral-7B (no internet required)

---

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.16 |
| C++ compiler | GCC ≥ 11 or Clang ≥ 13 or MSVC 2022 |
| Git | any recent |
| Disk space | ~5 GB for the GGUF model |

---

## Build

```bash
# 1. Clone
git clone https://github.com/yourname/student-expense-manager.git
cd student-expense-manager

# 2. Configure (CMake auto-fetches llama.cpp)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build build --parallel

# 4. Binary is at
./build/expense_manager
```

---

## Download the Model

```bash
mkdir -p models

# Option A — Hugging Face CLI
pip install huggingface_hub
huggingface-cli download TheBloke/Mistral-7B-Instruct-v0.2-GGUF \
    mistral-7b-instruct-v0.2.Q4_K_M.gguf \
    --local-dir models/

# Option B — direct wget (4.1 GB)
wget -P models/ \
  https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2.Q4_K_M.gguf

# Rename to match expected filename
mv models/mistral-7b-instruct-v0.2.Q4_K_M.gguf \
   models/mistral-7b-instruct-q4_k_m.gguf
```

---

## Run

```bash
./build/expense_manager
```

The program looks for the model at `models/mistral-7b-instruct-q4_k_m.gguf`
relative to the project root. CSV data is read from `data/expenses.csv`.

---

## Project Layout

```
student-expense-manager/
├── CMakeLists.txt
├── README.md
├── extern/
│   └── llama.cpp/          ← auto-fetched by CMake
├── models/
│   └── mistral-7b-instruct-q4_k_m.gguf
├── data/
│   └── expenses.csv
├── include/
│   ├── expense.h
│   ├── storage.h
│   ├── analytics.h
│   └── ai_advisor.h
├── src/
│   ├── main.cpp
│   ├── expense.cpp
│   ├── storage.cpp
│   ├── analytics.cpp
│   └── ai_advisor.cpp      ← only file that touches llama.cpp
└── reports/
    └── report_2026_05.txt
```

---

## CSV Format

`data/expenses.csv` uses a simple header row:

```
id,date,description,amount,category
1,2026-05-01,Lunch at canteen,85.00,Food
```

Categories: `Food`, `Transport`, `Entertainment`, `Utilities`, `Other`

---

## License

MIT