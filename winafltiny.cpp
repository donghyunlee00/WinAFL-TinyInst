/*
  References
  - https://github.com/googleprojectzero/TinyInst/blob/master/tinyinst-coverage.cpp
    - int main(int argc, char **argv)
    - void RunTarget(int argc, char **argv, unsigned int pid, uint32_t timeout)
  - https://github.com/linhlhq/TinyAFL/blob/master/afl-fuzz.cpp
    - void cook_coverage()
    - void move_coverage(u8* trace, Coverage cov_module)
  - https://github.com/googleprojectzero/winafl/blob/master/winaflpt.c
    - int pt_init(int argc, char **argv, char *module_dir)
    - int run_target_pt(char **argv, uint32_t timeout)
  - etc.
*/

#include "TinyInst/litecov.h"

#include "config.h"

#include "winafltiny.h"

extern "C" u8 *trace_bits;

int target_argc;
char **target_argv;
LiteCov *instrumentation;
bool persist;
int num_iterations;
int cur_iteration;

void move_coverage_tiny(Coverage cov_module) {
	for (auto iter = cov_module.begin(); iter != cov_module.end(); iter++) {
		for (auto iter1 = iter->offsets.begin(); iter1 != iter->offsets.end(); iter1++) {
			u32 index = *iter1 % MAP_SIZE;
			trace_bits[index] ++;
		}
	}
}

/* My way transform coverage of tinyinst to map coverage*/
void cook_coverage_tiny() {
	memset(trace_bits, 0, MAP_SIZE);
	Coverage newcoverage;
	instrumentation->GetCoverage(newcoverage, true);
	move_coverage_tiny(newcoverage);
}

// run a single iteration over the target process
// whether it's the whole process or target method
// and regardless if the target is persistent or not
// (should know what to do in pretty much all cases)
int run_target_tiny(uint32_t timeout) {
  DebuggerStatus status;
  int ret;

  if (instrumentation->IsTargetFunctionDefined()) {
    if (cur_iteration == num_iterations) {
      instrumentation->Kill();
      cur_iteration = 0;
    }
  }

  // else clear only when the target function is reached
  if (!instrumentation->IsTargetFunctionDefined()) {
    instrumentation->ClearCoverage();
  }

  if (instrumentation->IsTargetAlive() && persist) {
    status = instrumentation->Continue(timeout);
  } else {
    instrumentation->Kill();
    cur_iteration = 0;
    if (target_argc) {
      status = instrumentation->Run(target_argc, target_argv, timeout);
    } else {
      status = instrumentation->Attach(0, timeout);
    }
  }

  // if target function is defined,
  // we should wait until it is hit
  if (instrumentation->IsTargetFunctionDefined()) {
    if ((status != DEBUGGER_TARGET_START) && target_argc) {
      // try again with a clean process
      WARN("Target function not reached, retrying with a clean process\n");
      instrumentation->Kill();
      cur_iteration = 0;
      status = instrumentation->Run(target_argc, target_argv, timeout);
    }

    if (status != DEBUGGER_TARGET_START) {
      switch (status) {
      case DEBUGGER_CRASHED:
        FATAL("Process crashed before reaching the target method\n");
        break;
      case DEBUGGER_HANGED:
        FATAL("Process hanged before reaching the target method\n");
        break;
      case DEBUGGER_PROCESS_EXIT:
        FATAL("Process exited before reaching the target method\n");
        break;
      default:
        FATAL("An unknown problem occured before reaching the target method\n");
        break;
      }
    }

    instrumentation->ClearCoverage();

    status = instrumentation->Continue(timeout);
  }

  switch (status) {
  case DEBUGGER_CRASHED:
    printf("Process crashed\n");
    instrumentation->Kill();
    break;
  case DEBUGGER_HANGED:
    printf("Process hanged\n");
    instrumentation->Kill();
    break;
  case DEBUGGER_PROCESS_EXIT:
    if (instrumentation->IsTargetFunctionDefined()) {
      printf("Process exit during target function\n");
    } else {
      printf("Process finished normally\n");
    }
    break;
  case DEBUGGER_TARGET_END:
    if (instrumentation->IsTargetFunctionDefined()) {
      // printf("Target function returned normally\n");
      cur_iteration++;
    } else {
      FATAL("Unexpected status received from the debugger\n");
    }
    break;
  default:
    FATAL("Unexpected status received from the debugger\n");
    break;
  }

  if (status == DEBUGGER_PROCESS_EXIT) {
		ret = FAULT_TMOUT; //treat it as a hang
	} else if (status == DEBUGGER_HANGED) {
		ret = FAULT_TMOUT;
	} else if (status == DEBUGGER_CRASHED) {
		ret = FAULT_CRASH;
	} else if (status == DEBUGGER_TARGET_END) {
		ret = FAULT_NONE;
	}

  return ret;
}

int tiny_init(int argc, char **argv) {
  instrumentation = new LiteCov();
  instrumentation->Init(argc - 1, argv + 1);

  int target_opt_ind = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--") == 0) {
      target_opt_ind = i + 1;
      break;
    }
  }

  if (target_opt_ind <= 0) return 0;

  target_argc = (target_opt_ind) ? argc - target_opt_ind : 0;
  target_argv = (target_opt_ind) ? argv + target_opt_ind : NULL;

  persist = GetBinaryOption("-persist", argc - 1, argv + 1, false);
  num_iterations = GetIntOption("-iterations", argc - 1, argv + 1, 1);

  return target_opt_ind - 1;
}