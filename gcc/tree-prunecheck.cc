#define INCLUDE_MEMORY
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "tree.h"
#include "gimple.h"
#include "tree-pass.h"
#include "ssa.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "internal-fn.h"
#include "gimple-pretty-print.h"
#include "cgraph.h"
#include "hash-table.h" // GCC hash utilities
#include <string>        // For std::string
#include <unordered_map> // For std::unordered_map

// Pass metadata
const pass_data pass_data_prune_check = {
  GIMPLE_PASS, /* type */
  "prune", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_NONE, /* tv_id */
  PROP_cfg, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_prunecheck : public gimple_opt_pass {
public:
  pass_prunecheck(gcc::context *ctxt) : gimple_opt_pass(pass_data_prune_check, ctxt) {}

  /* opt_pass methods */
  bool gate(function *) final override {
    return 1; // Always execute pass
  }

  unsigned int execute(function *) final override;
};

unsigned int
pass_prunecheck::execute(function *fun)
{
  // Check if the function is a clone
  int isClone = DECL_FUNCTION_VERSIONED(fun->decl);

  if (dump_file) {
    fprintf(dump_file, "Function: %s\n", function_name(fun));
    if (isClone) {
      fprintf(dump_file, "Function is a clone/version: True\n");
    } else {
      fprintf(dump_file, "Function is a clone/version: False\n");
    }
  }

  // Initialize a hash value
  unsigned long hash = 5381;

  // Process all Gimple statements to compute the hash
  basic_block bb;
  FOR_EACH_BB_FN(bb, fun) {
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
      gimple *g = gsi_stmt(gsi);
      int code = gimple_code(g);

      // Update hash using a simple algorithm (DJB2-inspired)
      hash = ((hash << 5) + hash) + code; // hash * 33 + code
    }
  }

  // Static map to store function hashes
  static std::unordered_map<std::string, unsigned long> function_hashes;

  // Check if the hash already exists
  bool prune = false;
  for (const auto &entry : function_hashes) {
    if (entry.second == hash) {
      prune = true;
      break;
    }
  }

  // Print the hash and prune decision
  if (dump_file) {
    fprintf(dump_file, "Hash for function %s: %lu\n", function_name(fun), hash);
    if (prune) {
      fprintf(dump_file, "Decision: prune\n");
    } else {
      fprintf(dump_file, "Decision: no prune\n");
      // Add the current function's hash to the map
      function_hashes[function_name(fun)] = hash;
    }
  }

  if (dump_file) {
    fprintf(dump_file, "\n\n##### End function prune check, start regular dump of current gimple #####\n\n\n");
  }

  return 0;
}

// Entry point for GCC to register the pass
gimple_opt_pass *make_pass_prunecheck(gcc::context *ctxt) {
  return new pass_prunecheck(ctxt);
}
