#include "utf8/checked.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <iterator>
#include <nlohmann/json_fwd.hpp>
#include <streambuf>
#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

#include <sdl3/SDL.h>
#include <sdl3/SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include "ImGuiFileDialog.h"
#include "fpdfview.h"
#include "fpdf_text.h"

#include "llm.h"
#include "message.h"

static LLM& llm = LLM::instance();

typedef struct _user_state_t {
    //style
    const float rounding = 5.0f;

    //data
    std::vector<std::string> models;
    std::unordered_map<std::string, std::string> prompts;

    //messages view
    chat_messages_t chat_messages;

    ImVec2 current_cursor_pos{.0f, .0f};
    std::string edit_message = "";

    //config
    std::string model = "Qwen3-8B-Q4_K_M";
    float temperature = 0.6f;
    float top_p = 0.95f;
    int top_k = 20;
    float presence_penalty = 1.5f;
    std::string system_prompt = "";
} user_state_t;
static user_state_t user_state;

SDL_Window * ui_create(const nlohmann::json& config) {
    if (!SDL_Init(SDL_INIT_VIDEO)) { return nullptr; }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 
        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 
        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    int width = config.value("width", 960);
    int height = config.value("height", 640);
    std::string title = config.value("title", "Chat LLM");
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | 
        SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window * window = SDL_CreateWindow(title.c_str(), width, 
        height, flags);
    if (!window) {
        std::cout << std::format("SDL_CreateWindow error: {}\n", 
            SDL_GetError());
        SDL_Quit();
        return nullptr;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cout << std::format("SDL_GL_CreateContext error: {}\n", 
            SDL_GetError());
        SDL_Quit();
        return nullptr;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();
    
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    nlohmann::json config_fonts = config["fonts"];
    float font_size = config_fonts.value("size", 13.0f);
    std::string font_default = config_fonts.value("default", 
        "fonts/MonaspaceRadonVarVF[wght,wdth,slnt].ttf");
    auto font_addition = config_fonts.value("addition", 
        std::vector<nlohmann::json>{});

    ImFontConfig fc;
    fc.MergeMode = false;
    io.Fonts->AddFontFromFileTTF(font_default.c_str(), 
        font_size, &fc, 
        io.Fonts->GetGlyphRangesDefault());
    
    for (auto const& font: font_addition) {
        fc.MergeMode = true;
        if (font["language"] == "chinese") {
            io.Fonts->AddFontFromFileTTF(font["file"].get<std::string>().c_str(), 
            font_size, &fc, 
            io.Fonts->GetGlyphRangesChineseFull());
        } else if (font["language"] == "emoji") {
            ImWchar ranges[] = {0x1, 0x1FFFF, 0x0};
            fc.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;
            io.Fonts->AddFontFromFileTTF(font["file"].get<std::string>().c_str(), 
            font_size, &fc, 
            ranges);
        }
    }

    ImGui::GetStyle().FrameRounding = user_state.rounding;
    
    return window;
}

typedef void (* children)(const char *);
static auto box = [](const char * title, const ImVec2& pos, 
    const ImVec2& size, children c) {
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGuiWindowFlags flags;
    flags |= ImGuiWindowFlags_NoDecoration;
    ImGui::Begin(title, nullptr, flags);
    c(title);
    ImGui::End();
};

static auto chat_messages = [](const ImVec2& pos, 
        const ImVec2& size) {
    box("chat messages", pos, size, [](const char * title){
        ImGui::SeparatorText(title);
        ImGui::BeginChild("##messages", {0, 0}, 
            0, 
            ImGuiWindowFlags_AlwaysVerticalScrollbar);
        std::vector<chat_message_t> messages = 
            user_state.chat_messages.snapshot();
        for (int i=0; i<messages.size(); ++i) {
            auto const& message = messages[i];
            ImGui::Text("%s", message._time.c_str());
            if (message._role == "user") {
                ImGui::TextWrapped("%s", message._content.c_str());
            } else {
                if (message._reason.size() > 0) {
                    ImGui::PushStyleColor(ImGuiCol_Header, 
                        {0.f, 0.f, 0.f, 1.f});
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, 
                        {0.f, 0.f, 0.f, 1.f});
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 
                        {0.f, 0.f, 0.f, 1.f});
                    ImGui::PushStyleColor(ImGuiCol_Text, 
                        {1.0f, 0.7f, 0.8f, 1.0f});
                    std::string label = std::format("think##{}", i);
                    if (ImGui::CollapsingHeader(label.c_str())) {
                        ImGui::TextWrapped("%s", message._reason.c_str());
                    }
                    ImGui::PopStyleColor(4);
                }

                if (message._content.size() > 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, 
                        {0.9f, 0.5f, 0.5f, 1.0f});
                    ImGui::TextWrapped("%s", message._content.c_str());
                    ImGui::PopStyleColor();
                }
            }
            ImGui::Spacing();ImGui::Spacing();
        }

        float scroll_y = ImGui::GetScrollY();
        float scroll_max_y = ImGui::GetScrollMaxY();
        if ((scroll_max_y - scroll_y) < 1.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    });
};

std::vector<std::string> split_words(const std::string& s) {
    const int max_word_len = 16;
    std::vector<std::string> words;
    std::istringstream iss(s);
    std::string word;

    auto push_word = [&]() {
        if (word.size() > 0) {
            words.push_back(word);
            word.clear();
        }
    };

    while (!iss.eof()) {
        char c = (char)iss.peek();
        if ((c & 0x80) == 0x00) { //1
            iss.read(&c, 1); word.push_back(c);
            if (c == ' ' || word.size() > max_word_len) {
                push_word();
            }
        } else if ((c & 0xe0) == 0xc0) { //2
            push_word();
            word.assign(2, 0x00);
            iss.read(word.data(), 2);
            push_word();
        } else if ((c & 0xf0) == 0xe0) { //3
            push_word();
            word.assign(3, 0x00);
            iss.read(word.data(), 3);
            push_word();
        } else if ((c & 0xf8) == 0xf0) { //4
            push_word();
            word.assign(4, 0x00);
            iss.read(word.data(), 4);
            push_word();
        } else {
            push_word();
            iss.read(&c, 1);
        }
    }
    push_word();

    return words;
}

static auto restore_string = [](const char * s) {
    std::string buffer;
    const char * start = s;
    for (const char * end = std::strstr(start, " \n"); 
         end != nullptr; 
         start = end + strlen(" \n"), end = std::strstr(start, " \n")) {
        buffer += std::string(start, end - start);
    }
    buffer += std::string(start);
    return buffer;
};

static int chat_message_edit_callback(ImGuiInputTextCallbackData * data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        int max_width = ImGui::GetItemRectSize().x - ImGui::CalcTextSize(" \n").x;
        std::string buffer = restore_string(data->Buf);
        std::istringstream iss(buffer);
        std::ostringstream oss;
        bool dirty = false;
        bool at_end = (data->CursorPos == data->BufTextLen);

        for (std::string line; std::getline(iss, line);) {
            int width = ImGui::CalcTextSize(line.c_str()).x;
            if (width > max_width) {
                std::vector<std::string> words = split_words(line);
                std::string s, new_line;
                width = 0;
                for (auto const& word: words) {
                    int word_width = ImGui::CalcTextSize(word.c_str()).x;
                    width += word_width;
                    if (width >= max_width) {
                        if (new_line.size() > 0) new_line += " \n";
                        new_line += s;
                        s.clear();
                        width = word_width;
                    }
                    s += word;
                }
                if (s.size() > 0) {
                    if (new_line.size() > 0) new_line += " \n";
                    new_line += s;
                }
                if (oss.str().size() > 0) oss << std::endl;
                oss << new_line;
                dirty = true;
                continue;
            }
            if (oss.str().size() > 0) oss << std::endl;
            oss << line;
        }
        if (buffer.back() == '\n') oss << std::endl;

        if (dirty) {
            strcpy(data->Buf, oss.str().c_str());
            data->BufTextLen = strlen(data->Buf);
            if (at_end) data->CursorPos = data->BufTextLen;
            data->BufDirty = dirty;
        }
    }

    std::string buffer(data->Buf);
    buffer = buffer.substr(0, data->CursorPos);
    ImVec2 size = ImGui::CalcTextSize(buffer.c_str());
    ImVec2 line_size = size;
    int n_lines = std::count(buffer.begin(), 
        buffer.end(), '\n');
    if (n_lines > 0) {
        buffer = buffer.substr(buffer.rfind("\n") + 1);
        line_size = ImGui::CalcTextSize(buffer.c_str());
    }
    ImVec2 pos = ImGui::GetCursorScreenPos();
    user_state.current_cursor_pos = {
        pos.x + line_size.x, 
        pos.y + size.y - line_size.y
    };

    return 0;
}

static auto show_edit_message = []() {
    if (!user_state.edit_message.size()) return;
    ImDrawList * draw_list = ImGui::GetForegroundDrawList();
    auto pos = user_state.current_cursor_pos;
    auto size = ImGui::CalcTextSize(user_state.edit_message.c_str());
    draw_list->AddRectFilled(pos, 
        {pos.x + size.x, pos.y + size.y}, 
        IM_COL32(0, 0, 0, 255));
    draw_list->AddText(pos, IM_COL32(255, 255, 255, 255), 
        user_state.edit_message.c_str());
};

static auto chat_message = [](const ImVec2& pos, 
        const ImVec2& size) {
    box("chat message", pos, size, [](const char * title){
        (void)title;
        ImGui::BeginDisabled(!llm.llm_idle());
        
        if (user_state.current_cursor_pos.x == .0f && 
            user_state.current_cursor_pos.y == .0f) {
            user_state.current_cursor_pos = ImGui::GetCursorScreenPos();
        }

        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 
            {0.8f, 0.8f, 0.8f, 0.2f});
        char buf[2 * 1024] = {0x0};
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
        flags |= ImGuiInputTextFlags_CtrlEnterForNewLine;
        flags |= ImGuiInputTextFlags_NoHorizontalScroll;
        flags |= ImGuiInputTextFlags_CallbackAlways;
        flags |= ImGuiInputTextFlags_CallbackEdit;
        if (ImGui::InputTextMultiline("##message", buf, IM_ARRAYSIZE(buf), 
            {size.x - 20, size.y}, flags, chat_message_edit_callback)) {
            if (strlen(buf) > 0) {
                nlohmann::json request;
                request["model"] = user_state.model;
                request["temperature"] = user_state.temperature;
                request["top_p"] = user_state.top_p;
                request["top_k"] = user_state.top_k;
                request["presence_penalty"] = user_state.presence_penalty;
                request["messages"] = {
                    {{"role", "system"}, 
                        {"content", user_state.system_prompt}},
                    {{"role", "user"}, 
                        {"content", restore_string(buf)}}
                };
                llm.generate(request);

                chat_message_t message{"user", buf};
                user_state.chat_messages.push(message);
                buf[0] = '\0';
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("+")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 1;
            config.flags |= ImGuiFileDialogFlags_DontShowHiddenFiles;
            config.flags |= ImGuiFileDialogFlags_DisableCreateDirectoryButton;
            config.flags |= ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDialog", 
                "Choose a file", ".pdf,.txt", config);
        }
        ImGui::EndDisabled();
        show_edit_message();
    });
};

static auto llama = [](const ImVec2& pos, 
        const ImVec2& size) {
    box("llm", pos, size, [](const char * title){
        ImGui::SeparatorText(title);

        ImGui::Text("Server: %s", llm.llm_base_url().c_str());
        ImGui::Spacing();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();

        ImU32 col = llm.llm_running() ? IM_COL32(0, 255, 0, 255) : 
            IM_COL32(128, 128, 128, 255);
        ImGui::GetWindowDrawList()->AddRectFilled(pos, 
            {pos.x + size.x, pos.y + 5.0f}, col, 
            user_state.rounding);
        ImGui::Dummy({size.x, 5.0f});
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Model:");
        ImGui::SetNextItemWidth(size.x);
        const char * preview_model = user_state.model.c_str();
        if (ImGui::BeginCombo("##models", preview_model)) {
            for (auto const& model: user_state.models) {
                bool is_selected = (model == user_state.model);
                if (ImGui::Selectable(model.c_str(), is_selected)) {
                    user_state.model = model;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Text("Temperature:");
        ImGui::SetNextItemWidth(size.x);
        ImGui::DragFloat("##temperature", &user_state.temperature, 
            0.1f, 0.0f, 2.0f, "%.1f");
        ImGui::Text("Top-p:");
        ImGui::SetNextItemWidth(size.x);
        ImGui::DragFloat("##top_p", &user_state.top_p, 
            0.01f, 0.00f, 1.00f, "%.2f");
        ImGui::Text("Top-k:");
        ImGui::SetNextItemWidth(size.x);
        ImGui::DragInt("##top_k", &user_state.top_k, 
            1, 1, 100, "%d");
        ImGui::Text("Presence Penalty:");
        ImGui::SetNextItemWidth(size.x);
        ImGui::DragFloat("##presence_penalty", &user_state.presence_penalty, 
            0.1f, -2.0f, 2.0f, "%.1f");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SeparatorText("System Prompt");
        ImGui::SetNextItemWidth(size.x);
        static std::string prompt_selected = "default";
        const char * preview_prompt = prompt_selected.c_str();
        if (ImGui::BeginCombo("##prompts", preview_prompt)) {
            for (auto const& [key, value]: user_state.prompts) {
                bool is_selected = (key == prompt_selected);
                if (ImGui::Selectable(key.c_str(), is_selected)) {
                    prompt_selected = key;
                    user_state.system_prompt = value;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PushStyleColor(ImGuiCol_FrameBg, 
            ImVec4{.8f, .8f, .8f, .2f});
        ImGui::BeginChild("##system-prompt", 
            {0, 0}, 
            ImGuiChildFlags_FrameStyle, 
            ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::TextWrapped("%s", user_state.system_prompt.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
    });
};

std::string load_txt_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    return std::string(std::istreambuf_iterator<char>(f), 
        std::istreambuf_iterator<char>());
}

std::string load_pdf_file(const std::string& path) {
    std::string content;
    FPDF_DOCUMENT doc = FPDF_LoadDocument(path.c_str(), NULL);
    if (doc) {
        int n_pages = FPDF_GetPageCount(doc);
        for (int i=0; i<n_pages; ++i) {
            FPDF_PAGE page = FPDF_LoadPage(doc, i);
            if (page == nullptr) continue;
            FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
            if (text_page) {
                int len = FPDFText_CountChars(text_page);
                std::vector<unsigned short> u16_buffer(len + 1);
                if (FPDFText_GetText(text_page, 0, 
                        len, u16_buffer.data()) > 0) {
                    std::string u8_buffer;
                    utf8::utf16to8(u16_buffer.cbegin(), u16_buffer.cend(), 
                        std::back_inserter(u8_buffer));
                    content += u8_buffer;
                }

                FPDFText_ClosePage(text_page);
            }

            FPDF_ClosePage(page);
        }

        FPDF_CloseDocument(doc);
    }
    return content;
}

std::string load_file(const std::string& path) {
    if (path.ends_with(".pdf")) return load_pdf_file(path);
    return load_txt_file(path);
}

static auto choose_file = [](ImVec2 size) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoScrollbar;
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDialog", 
        flags, size)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            //std::cout << path << std::endl;
            std::string content = load_file(path);
            //std::cout << "content: " << content << std::endl;
            if (content.size() > 0) {
                nlohmann::json request;
                request["model"] = user_state.model;
                request["temperature"] = user_state.temperature;
                request["top_p"] = user_state.top_p;
                request["top_k"] = user_state.top_k;
                request["presence_penalty"] = user_state.presence_penalty;
                request["messages"] = {
                    {{"role", "system"}, 
                        {"content", user_state.system_prompt}},
                    {{"role", "user"}, 
                        {"content", content}}
                };
                llm.generate(request);

                int length = 512;
                if (content.size() > length) {
                    for (; length>0; --length) {
                        if ((content[length - 1] & 0x80) != 0x80) {
                            break;
                        }
                        if (((content[length - 1] & 0xc0) == 0xc0) || 
                            ((content[length - 1] & 0xe0) == 0xe0) || 
                            ((content[length - 1] & 0xf0) == 0xf0)) {
                            --length;
                            break;
                        }
                    }
                    content = content.substr(0, length) + "...";
                }
                chat_message_t message{"user", content};
                user_state.chat_messages.push(message);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
};

void ui_update(SDL_Window * window) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);

    chat_messages({.0f, .0f}, 
        {width * 0.7f, height * 0.8f});
    chat_message({.0f, height * 0.8f}, 
        {width * 0.7f, height * 0.2f});
    llama({width * 0.7f, .0f}, 
        {width * 0.3f, height * 1.0f});
    choose_file({width * 0.6f, height * 0.5f});
}

void list_models(std::vector<std::string>& m) {
    const std::filesystem::path path{"models"};
    for (auto const& item: 
            std::filesystem::directory_iterator{path}) {
        if (item.is_regular_file() && item.path().extension() == ".gguf") {
            m.push_back(item.path().stem());
        }
    }
    if (m.size() > 0) user_state.model = m[0];
}

void list_prompts(std::unordered_map<std::string, std::string>& m) {
    const std::filesystem::path path{"prompts"};
    for (auto const& item: 
            std::filesystem::directory_iterator{path}) {
        if (item.is_regular_file() && item.path().extension() == ".txt") {
            std::ifstream f(item.path());
            if (f.is_open()) {
                std::string key = item.path().stem();
                std::string value{
                    std::istreambuf_iterator<char>(f), 
                    std::istreambuf_iterator<char>()
                };
                m[key] = value;
                f.close();
            }
        }
    }

    if (!m.contains("default")) {
        m["default"] = "You are a helpful assistant.";
    }
    user_state.system_prompt = m["default"];
}

int main(int argc, char *argv[]) {
    std::ofstream out("log.txt");
    std::streambuf * cout_buf = std::cout.rdbuf();

    namespace po = boost::program_options;
    po::options_description desc("Allowed Options");
    desc.add_options()
        ("help,h", "show help message.")
        ("config,c", 
            po::value<std::string>()->default_value("config/config.json"),
            "set config file.")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help") > 0) {
        std::cout << desc << std::endl;
        return 0;
    }
    nlohmann::json config;
    {
        std::ifstream f(vm["config"].as<std::string>());
        if (!f.is_open()) {
            std::cout << "config file not found." << std::endl;
            return -1;
        }
        f >> config;
        f.close();
    }

    bool verbose = config.value("verbose", false);
    if (verbose) std::cout.rdbuf(out.rdbuf());

    list_models(user_state.models);
    list_prompts(user_state.prompts);

    FPDF_InitLibrary();

    SDL_Window * window = ui_create(config["ui"]);
    if (!window) {
        std::cout << "fail to create window." << std::endl;
        return -1;
    }

    //test data
    //set_test_data(user_state.chat_messages);
    llm.init(config["llm"], 
        [](const std::string& result){
            chat_message_t message {"assistant", result};
            user_state.chat_messages.push(message);
        }, verbose);

    bool done = false;
    while (!done) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT) done = true;
            if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && 
                ev.window.windowID == SDL_GetWindowID(window)) done = true;
            if (ev.type == SDL_EVENT_TEXT_EDITING) {
                user_state.edit_message = ev.edit.text;
//                user_state.edit_message.erase(
//                    std::remove(user_state.edit_message.begin(), 
//                        user_state.edit_message.end(), ' '), 
//                    user_state.edit_message.end());
            }
            if (ev.type == SDL_EVENT_TEXT_INPUT) {
                user_state.edit_message.clear();
            }
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(100);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ui_update(window);

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    llm.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(SDL_GL_GetCurrentContext());
    SDL_DestroyWindow(window);
    SDL_Quit();

    FPDF_DestroyLibrary();

    std::cout << "main exit!" << std::endl;
    if (verbose) std::cout.rdbuf(cout_buf);
    return 0;
}