# 💬 chat.llm: Modular C++ LLM Chat Interface for Documents and Beyond

**chat.llm** is a lightweight, extensible C++ desktop application that enables LLM-powered interaction with documents — and much more. Chat with your `.pdf` and `.txt` files, generate summaries, translate content, or switch to travel/meal planner mode — all through modular system prompts.

Built with [ImGui](https://github.com/ocornut/imgui), [llama.cpp](https://github.com/ggerganov/llama.cpp), [PDFium](https://pdfium.googlesource.com/pdfium), and [utfcpp](https://github.com/nemtrif/utfcpp) for robust Unicode handling.

---

## ✨ Key Features

- 🧠 **LLM Chat Engine** via `llama.cpp`
- 📄 **Document Chat**: Interact with `.pdf` and `.txt` files
- 🌍 **UTF-8 / Unicode Support**: Reliable multilingual processing using `utfcpp`
- ⚙️ **Configurable System Prompts**:
  - 📚 Summarization
  - 🌐 Translation (e.g., English ⇌ Chinese)
  - 🧳 Travel planning
  - 🍱 Recipe generation
  - 💬 General Q&A
- 🧩 **Minimalist GUI** using ImGui
- 💻 **Cross-platform** C++17 codebase

---

## 🛠️ Example Use Cases

| Mode              | Description                                                |
|-------------------|------------------------------------------------------------|
| Summarizer        | Summarize long PDF/TXT documents in natural language       |
| Translator        | Translate documents between multiple languages             |
| Travel Planner    | Generate trip plans based on user input                    |
| Recipe Assistant  | Plan healthy meals for families or individuals             |
| Freeform Chat     | General assistant/chat mode using your own system prompt   |

Switching between modes is as simple as changing the `system_prompt`.

---

## 🔧 Getting Started

### Requirements

- C++17-compatible compiler
- CMake 3.15+
- llama.cpp-compatible LLM (quantized `.gguf` model)
- PDFium for PDF support

### Build Instructions

```bash
git clone https://github.com/szsteven008/chat.llm.git
cd chat.llm
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make -j
```

## 📦 Dependencies
	•	llama.cpp — local LLM inference
	•	ImGui — GUI framework
	•	PDFium — PDF parsing
	•	utfcpp — UTF-8 string handling (for Chinese and multilingual support. licensed under Boost Software License 1.0)

## 🖼️ Screenshot
<img width="964" alt="image" src="https://github.com/user-attachments/assets/1583a720-14e6-4271-ba31-b5c9ce5539ef" />

## 📄 License
MIT License. 

## ❤️ Credits
• llama.cpp by Georgi Gerganov
• Qwen3 by Alibaba Group
• Dear ImGui by Omar Cornut
• json by Niels Lohmann
• utfcpp by Nemanja Trifunovic
