#include "../include/AIadviser.h"
#include "llama.h"
#include <iostream>
#include <vector>

struct AIAdvisor::LlamaState {
    llama_model*   model   = nullptr;
    llama_context* ctx     = nullptr;
    llama_sampler* sampler = nullptr;
};

AIAdvisor::AIAdvisor(const std::string& modelPath, int nCtx)
    : m_state(new LlamaState())
{
    llama_backend_init();

    llama_model_params mp = llama_model_default_params();
    m_state->model = llama_load_model_from_file(modelPath.c_str(), mp);
    if (!m_state->model) {
        std::cerr << "  [!] Cannot load model: " << modelPath << "\n";
        return;
    }

    llama_context_params cp = llama_context_default_params();
    cp.n_ctx = nCtx;
    m_state->ctx = llama_new_context_with_model(m_state->model, cp);
    if (!m_state->ctx) {
        std::cerr << "  [!] Cannot create context.\n";
        return;
    }

    // NEW: build a greedy sampler chain (replaces llama_sample_token_greedy)
    m_state->sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(m_state->sampler, llama_sampler_init_greedy());

    m_loaded = true;
}

AIAdvisor::~AIAdvisor() {
    if (m_state) {
        if (m_state->sampler) llama_sampler_free(m_state->sampler);
        if (m_state->ctx)     llama_free(m_state->ctx);
        if (m_state->model)   llama_free_model(m_state->model);
        delete m_state;
    }
    llama_backend_free();
}

std::string AIAdvisor::buildPrompt(const std::string& summary) const {
    return "<|system|>\nYou are a financial advisor for a student.</s>\n<|user|>\nGive 3 short money saving tips based on this:\n" + summary + "</s>\n<|assistant|>\n";
}
std::string AIAdvisor::query(const std::string& expenseSummary,
                              std::function<void(const std::string&)> tokenCallback)
{
    if (!m_loaded) return "";

    std::string prompt = buildPrompt(expenseSummary);

    // NEW: llama_tokenize now takes llama_vocab* instead of llama_model*
    const llama_vocab* vocab = llama_model_get_vocab(m_state->model);

    std::vector<llama_token> tokens(prompt.size() + 64);
    int n = llama_tokenize(vocab, prompt.c_str(), (int)prompt.size(),
                           tokens.data(), (int)tokens.size(), true, true);
    if (n < 0) return "";
    tokens.resize(n);

    llama_batch batch = llama_batch_get_one(tokens.data(), n);
    if (llama_decode(m_state->ctx, batch) != 0) return "";

    std::string result;

    // NEW: llama_token_eos and llama_token_to_piece also take llama_vocab*
    llama_token eos = llama_token_eos(vocab);

    for (int i = 0; i < m_maxTokens; ++i) {
        // NEW: llama_sampler_sample replaces llama_sample_token_greedy
        llama_token next = llama_sampler_sample(m_state->sampler, m_state->ctx, -1);

        if (next == eos) break;

        char buf[64];
        // NEW: llama_token_to_piece takes llama_vocab* instead of llama_model*
        int len = llama_token_to_piece(vocab, next, buf, sizeof(buf), 0, true);
        if (len > 0) {
            std::string piece(buf, len);
            result += piece;
            if (tokenCallback) tokenCallback(piece);
        }

        llama_batch nb = llama_batch_get_one(&next, 1);
        if (llama_decode(m_state->ctx, nb) != 0) break;
    }

    return result;
}