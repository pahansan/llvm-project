#ifndef LLVM_TRANSFORMS_UTILS_LOOPPEELWITHSTATE_H
#define LLVM_TRANSFORMS_UTILS_LOOPPEELWITHSTATE_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

namespace llvm {

class Loop;
class LPMUpdater;

class LoopPeelWithStatePass : public PassInfoMixin<LoopPeelWithStatePass> {
public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

private:
  bool hasStateVariables(Loop &L);
  bool isDerivedFromIndVar(Value *DerivedValue, Loop &L);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_LOOPPEELWITHSTATE_H
