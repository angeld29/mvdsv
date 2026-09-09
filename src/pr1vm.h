/*
pr1vm.h -- PR1 engine instance (mvdsv PR1 multi-instance sync, W1/S1)

The PR1 interpreter is moved from global state to per-instance state:
execution state (call stack, local stack, xfunction/xstatement) lives in
pr1vm_t. The "module" globals (progs/pr_functions/...) are shared between
PR1 and PR2 and read by external code (sv_*.c) — they stay global; pr1vm_t
mirrors them via PR1VM_BindServer() before each server call and they are
fully moved onto the instance in later slices (W2-W4 / ezq S2-S4).

Note: mvdsv is SERVERONLY — the server instance (sv_pr1vm) is the only one
used; the client CSQC VM pieces stay in ezquake (see the sync plan
docs/mvdsv_pr1vm_multiinstance_plan.md). Mirrors/lumps equal the shared
server globals, so PR_* wrappers keep existing behavior.

Ported from ezquake csqc/pr1vm-engine (slice S1, de083a87); comments in
English per engine convention (AGENTS.md).
*/

#ifndef PR1VM_H
#define PR1VM_H

#include "progs.h"	// dprograms_t/dstatement_t/..., edict_t, globalvars_t

#define PR1VM_MAX_STACK	32
#define PR1VM_LOCALSTACK	2048

typedef struct pr1vm_s pr1vm_t;

typedef struct
{
	int			s;			// statement (return address)
	dfunction_t	*f;			// function
} pr1vm_stack_t;

struct pr1vm_s
{
	// Mirrors of the shared "module" globals (server = global symbols via
	// BindServer; mirrors become real buffers on instance in later slices).
	dprograms_t		*progs;
	dfunction_t		*functions;
	ddef_t			*fielddefs;
	ddef_t			*globaldefs;
	dstatement_t	*statements;
	char			*strings;
	globalvars_t	*global_struct;
	float			*globals;
	int				edict_size;		// bytes per entvars block

	// Edict model (server: sv.edicts / sv.game_edicts).
	edict_t			*edicts;
	int				num_edicts;
	int				max_edicts;
	void			*game_edicts;	// entvars base for STOREP_*/EDICT_TO_PROG

	// Execution state (moved from pr_exec.c globals).
	pr1vm_stack_t	stack[PR1VM_MAX_STACK];
	int				depth;
	int				localstack[PR1VM_LOCALSTACK];
	int				localstack_used;
	dfunction_t		*xfunction;
	int				xstatement;

	// Host interface (filled in W4/S4; reserved here).
	void (*host_error)(pr1vm_t *vm, const char *fmt, ...);
	void (*host_print)(pr1vm_t *vm, const char *fmt, ...);
	void *host_udata;
};

// Active instance (the one PR1 is currently executing inside; NULL outside).
pr1vm_t *PR1VM_Active(void);

// Server instance (sv_pr1vm) — default target of the PR_* wrappers.
pr1vm_t *PR1VM_Server(void);

// Zero the instance and (for the server) fill mirrors from shared globals+sv.*.
void PR1VM_Reset(pr1vm_t *vm);
void PR1VM_BindServer(pr1vm_t *vm);

// Load: byte-swap header+lumps and fill the instance mirrors (without
// version/CRC validation — done by the server wrapper).
void PR1VM_LoadData(pr1vm_t *vm, dprograms_t *hdr);
// Server: instance mirrors -> shared "module" globals (read by PR2/sv_*.c).
void PR1VM_CommitServer(pr1vm_t *vm);

int  PR1VM_EnterFunction(pr1vm_t *vm, dfunction_t *f);
int  PR1VM_LeaveFunction(pr1vm_t *vm);
void PR1VM_ExecuteProgram(pr1vm_t *vm, func_t fnum);

#endif /* PR1VM_H */
