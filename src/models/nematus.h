#pragma once

#include "data/corpus.h"
#include "graph/expression_graph.h"
#include "layers/rnn.h"
#include "layers/param_initializers.h"
#include "layers/generic.h"
#include "3rd_party/cnpy/cnpy.h"

namespace marian {

class Nematus : public ExpressionGraph {
  public:
    Nematus()
    : dimSrcVoc_(40000), dimSrcEmb_(512), dimEncState_(1024),
      dimTrgVoc_(40000), dimTrgEmb_(512), dimDecState_(1024),
      dimBatch_(40) {}

    Nematus(const std::vector<int> dims)
    : dimSrcVoc_(dims[0]), dimSrcEmb_(dims[1]), dimEncState_(dims[2]),
      dimTrgVoc_(dims[3]), dimTrgEmb_(dims[4]), dimDecState_(dims[5]),
      dimBatch_(dims[6]) {}

  private:
    int dimSrcVoc_;
    int dimSrcEmb_;
    int dimEncState_;

    int dimTrgVoc_;
    int dimTrgEmb_;
    int dimDecState_;

    int dimBatch_;

    void setDims(Ptr<ExpressionGraph> graph,
                 Ptr<data::CorpusBatch> batch) {
      dimSrcVoc_ = graph->get("Wemb") ? graph->get("Wemb")->shape()[0] : dimSrcVoc_;
      dimSrcEmb_ = graph->get("Wemb") ? graph->get("Wemb")->shape()[1] : dimSrcEmb_;
      dimEncState_ = graph->get("encoder_U") ? graph->get("encoder_U")->shape()[0] : dimEncState_;

      dimTrgVoc_ = graph->get("Wemb_dec") ? graph->get("Wemb_dec")->shape()[0] : dimTrgVoc_;
      dimTrgEmb_ = graph->get("Wemb_dec") ? graph->get("Wemb_dec")->shape()[1] : dimTrgEmb_;
      dimDecState_ = graph->get("decoder_U") ? graph->get("decoder_U")->shape()[0] : dimDecState_;

      dimBatch_ = batch->size();
    }

  public:

    void load(Ptr<ExpressionGraph> graph,
              const std::string& name) {
      using namespace keywords;

      auto numpy = cnpy::npz_load(name);

      auto parameters = {
        // Source word embeddings
        "Wemb",

        // GRU in encoder
        "encoder_U", "encoder_W", "encoder_b",
        "encoder_Ux", "encoder_Wx", "encoder_bx",

        // GRU in encoder, reversed
        "encoder_r_U", "encoder_r_W", "encoder_r_b",
        "encoder_r_Ux", "encoder_r_Wx", "encoder_r_bx",

        // Transformation of decoder input state
        "ff_state_W", "ff_state_b",

        // Target word embeddings
        "Wemb_dec",

        // GRU layer 1 in decoder
        "decoder_U", "decoder_W", "decoder_b",
        "decoder_Ux", "decoder_Wx", "decoder_bx",

        // Attention
        "decoder_W_comb_att", "decoder_b_att",
        "decoder_Wc_att", "decoder_U_att",

        // GRU layer 2 in decoder
        "decoder_U_nl", "decoder_Wc", "decoder_b_nl",
        "decoder_Ux_nl", "decoder_Wcx", "decoder_bx_nl",

        // Read out
        "ff_logit_lstm_W", "ff_logit_lstm_b",
        "ff_logit_prev_W", "ff_logit_prev_b",
        "ff_logit_ctx_W", "ff_logit_ctx_b",
        "ff_logit_W", "ff_logit_b",
      };

      std::map<std::string, std::string> nameMap = {
        {"decoder_U", "decoder_cell1_U"},
        {"decoder_W", "decoder_cell1_W"},
        {"decoder_b", "decoder_cell1_b"},
        {"decoder_Ux", "decoder_cell1_Ux"},
        {"decoder_Wx", "decoder_cell1_Wx"},
        {"decoder_bx", "decoder_cell1_bx"},

        {"decoder_U_nl", "decoder_cell2_U"},
        {"decoder_Wc", "decoder_cell2_W"},
        {"decoder_b_nl", "decoder_cell2_b"},
        {"decoder_Ux_nl", "decoder_cell2_Ux"},
        {"decoder_Wcx", "decoder_cell2_Wx"},
        {"decoder_bx_nl", "decoder_cell2_bx"},

        {"ff_logit_prev_W", "ff_logit_l1_W0"},
        {"ff_logit_prev_b", "ff_logit_l1_b0"},
        {"ff_logit_lstm_W", "ff_logit_l1_W1"},
        {"ff_logit_lstm_b", "ff_logit_l1_b1"},
        {"ff_logit_ctx_W", "ff_logit_l1_W2"},
        {"ff_logit_ctx_b", "ff_logit_l1_b2"},

        {"ff_logit_W", "ff_logit_l2_W"},
        {"ff_logit_b", "ff_logit_l2_b"}
      };

      for(auto name : parameters) {
        Shape shape;
        if(numpy[name].shape.size() == 2) {
          shape.set(0, numpy[name].shape[0]);
          shape.set(1, numpy[name].shape[1]);
        }
        else if(numpy[name].shape.size() == 1) {
          shape.set(0, 1);
          shape.set(1, numpy[name].shape[0]);
        }

        std::string pName = name;
        if(nameMap.count(name))
          pName = nameMap[name];

        graph->param(pName, shape,
                     init=inits::from_numpy(numpy[name]));
      }
    }

    void save(Ptr<ExpressionGraph> graph,
              const std::string& name) {

      unsigned shape[2];
      std::string mode = "w";

      std::map<std::string, std::string> nameMap = {
        {"decoder_cell1_U", "decoder_U"},
        {"decoder_cell1_W", "decoder_W"},
        {"decoder_cell1_b", "decoder_b"},
        {"decoder_cell1_Ux", "decoder_Ux"},
        {"decoder_cell1_Wx", "decoder_Wx"},
        {"decoder_cell1_bx", "decoder_bx"},

        {"decoder_cell2_U", "decoder_U_nl"},
        {"decoder_cell2_W", "decoder_Wc"},
        {"decoder_cell2_b", "decoder_b_nl"},
        {"decoder_cell2_Ux", "decoder_Ux_nl"},
        {"decoder_cell2_Wx", "decoder_Wcx"},
        {"decoder_cell2_bx", "decoder_bx_nl"},

        {"ff_logit_l1_W0", "ff_logit_prev_W"},
        {"ff_logit_l1_b0", "ff_logit_prev_b"},
        {"ff_logit_l1_W1", "ff_logit_lstm_W"},
        {"ff_logit_l1_b1", "ff_logit_lstm_b"},
        {"ff_logit_l1_W2", "ff_logit_ctx_W"},
        {"ff_logit_l1_b2", "ff_logit_ctx_b"},

        {"ff_logit_l2_W", "ff_logit_W"},
        {"ff_logit_l2_b", "ff_logit_b"}
      };

      for(auto p : graph->params().getMap()) {
        std::vector<float> v;
        p.second->val() >> v;

        unsigned dim;
        if(p.second->shape()[0] == 1) {
          shape[0] = p.second->shape()[1];
          dim = 1;
        }
        else {
          shape[0] = p.second->shape()[0];
          shape[1] = p.second->shape()[1];
          dim = 2;
        }

        std::string pName = p.first;
        if(nameMap.count(pName))
          pName = nameMap[pName];

        cnpy::npz_save(name, pName, v.data(), shape, dim, mode);
        mode = "a";
      }

      float ctt = 0;
      shape[0] = 1;
      cnpy::npz_save(name, "decoder_c_tt", &ctt, shape, 1, mode);
    }

    std::tuple<Expr, Expr>
    prepareSource(Expr emb, Ptr<data::CorpusBatch> batch, size_t index) {
      using namespace keywords;
      std::vector<size_t> indeces;
      std::vector<float> mask;

      for(auto& word : (*batch)[index]) {
        for(auto i: word.first)
          indeces.push_back(i);
        for(auto m: word.second)
          mask.push_back(m);
      }

      int dimBatch = batch->size();
      int dimEmb = emb->shape()[1];
      int dimWords = (int)(*batch)[index].size();

      auto graph = emb->graph();
      auto x = reshape(rows(emb, indeces), {dimBatch, dimEmb, dimWords});
      auto xMask = graph->constant(shape={dimBatch, 1, dimWords},
                                   init=inits::from_vector(mask));
      return std::make_tuple(x, xMask);
    }

    std::tuple<Expr, Expr, Expr>
    prepareTarget(Expr emb, Ptr<data::CorpusBatch> batch, size_t index) {
      using namespace keywords;

      std::vector<size_t> indeces;
      std::vector<float> mask;
      std::vector<float> findeces;

      for(int j = 0; j < (*batch)[index].size(); ++j) {
        auto& trgWordBatch = (*batch)[index][j];

        for(auto i : trgWordBatch.first) {
          findeces.push_back((float)i);
          if(j < (*batch)[index].size() - 1)
            indeces.push_back(i);
        }

        for(auto m : trgWordBatch.second)
            mask.push_back(m);
      }

      int dimBatch = batch->size();
      int dimEmb = emb->shape()[1];
      int dimWords = (int)(*batch)[index].size();

      auto graph = emb->graph();

      auto y = reshape(rows(emb, indeces),
                       {dimBatch, dimEmb, dimWords - 1});

      auto yMask = graph->constant(shape={dimBatch, 1, dimWords},
                                  init=inits::from_vector(mask));
      auto yIdx = graph->constant(shape={(int)findeces.size(), 1},
                                  init=inits::from_vector(findeces));

      return std::make_tuple(y, yMask, yIdx);
    }

    Expr construct(Ptr<ExpressionGraph> graph,
                   Ptr<data::CorpusBatch> batch) {
      using namespace keywords;
      graph->clear();

      setDims(graph, batch);

      // Embeddings
      auto xEmb = Embedding("Wemb", dimSrcVoc_, dimSrcEmb_)(graph);
      auto yEmb = Embedding("Wemb_dec", dimTrgVoc_, dimTrgEmb_)(graph);

      Expr x, xMask;
      Expr y, yMask, yIdx;

      std::tie(x, xMask) = prepareSource(xEmb, batch, 0);
      std::tie(y, yMask, yIdx) = prepareTarget(yEmb, batch, 1);

      // Encoder
      auto xContext = BiRNN<GRU>("encoder", dimEncState_)
                        (x, mask=xMask);

      auto xMeanContext = weighted_average(xContext, xMask, axis=2);

      // Decoder
      auto yStart = Dense("ff_state",
                          dimDecState_,
                          activation=act::tanh)(xMeanContext);

      auto yEmpty = graph->zeros(shape={dimBatch_, dimTrgEmb_});
      auto yShifted = concatenate({yEmpty, y}, axis=2);
      //auto yShifted = shift(y, 1, axis=2);

      CGRU cgru({"decoder", xContext, dimDecState_, mask=xMask});
      auto yLstm = RNN<CGRU>("decoder", dimDecState_, cgru)
                     (yShifted, yStart);
      auto yCtx = cgru.getContexts();

      // 2-layer feedforward network for outputs and cost
      auto ff_logit_l1 = Dense("ff_logit_l1", dimTrgEmb_,
                               activation=act::tanh)
                           (yShifted, yLstm, yCtx);

      auto ff_logit_l2 = Dense("ff_logit_l2", dimTrgVoc_)
                           (ff_logit_l1);

      auto cost = CrossEntropyCost("cost")
                    (ff_logit_l2, yIdx, mask=yMask);

      return cost;
    }
};

}