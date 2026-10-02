/* Sunset Riders: continuous guest execution across frame and interrupt waits. */
#include "game_rtl.h"

#include <stdio.h>
#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/cart.h"
#include "snes/dma.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
#include "snes/snes.h"

extern CpuState g_cpu;
extern Ppu *g_ppu;

#define GAME_MASTER_CYCLES_PER_FRAME 357368ull
#define GAME_MAX_SLICES 4096

static uint32_t g_resume_pc;
static int g_waiting;
static int g_execution_failed;

static uint32_t read_vector(uint32_t addr)
{
    uint32_t lo = snes_read(g_snes, addr);
    uint32_t hi = snes_read(g_snes, addr + 1u);
    return (hi << 8) | lo;
}

/* Enter an interrupt, but keep it on the same resumable guest stream.
 * Sunset Riders can wait for another NMI while still inside the previous
 * NMI. Running each handler atomically through RTI deadlocks that wait.
 * The real RTI instruction restores the interrupted PC and stack in the
 * whole-program bridge; the host never discards an unfinished handler. */
static void enter_interrupt(uint32_t vector)
{
    cpu_push_interrupt_frame_at(&g_cpu, g_resume_pc);
    g_cpu._flag_I = 1;
    g_cpu._flag_D = 0;
    cpu_mirrors_to_p(&g_cpu);
    g_resume_pc = read_vector(vector);
    g_waiting = 0;
}

static void idle_until(uint64_t deadline)
{
    uint64_t target = deadline;
    uint32_t irq_clocks;

    snes_sync_master_clock(g_snes, g_cpu.master_cycles);
    irq_clocks = snes_master_clocks_until_irq(g_snes);
    if (irq_clocks && !g_cpu._flag_I &&
        g_cpu.master_cycles + irq_clocks + 1u < target)
        target = g_cpu.master_cycles + irq_clocks + 1u;

    g_cpu.master_cycles = target;
    g_cpu.coprocessor_master_cycles = target;
    snes_sync_master_clock(g_snes, target);
    if (g_snes->cart)
        cart_sync_coprocessors(g_snes->cart, target);
}

static void run_until(uint64_t deadline)
{
    int slice;
    for (slice = 0; slice < GAME_MAX_SLICES &&
         g_cpu.master_cycles < deadline; ++slice) {
        int ok, parked;
        uint32_t resume;

        if (g_snes->inIrq && !g_cpu._flag_I)
            enter_interrupt(g_cpu.emulation ? 0x00FFFEu : 0x00FFEEu);

        if (g_waiting) {
            idle_until(deadline);
            continue;
        }

        interp_bridge_set_master_deadline(deadline);
        ok = interp_bridge_run_until_quiescent(&g_cpu, g_resume_pc);
        interp_bridge_set_master_deadline(0);
        resume = interp_bridge_lle_resume_pc();
        if (resume)
            g_resume_pc = resume;

        g_waiting = interp_bridge_lle_took_wai();
        parked = interp_bridge_lle_took_quiescent();
        if (!ok || !resume) {
            fprintf(stderr, "[sunset] execution stopped: pc=$%06X S=$%04X\n",
                    (unsigned)g_resume_pc, (unsigned)g_cpu.S);
            g_execution_failed = 1;
            return;
        }
        if (g_snes->inIrq && !g_cpu._flag_I)
            continue;
        if (g_cpu.master_cycles < deadline && (g_waiting || parked))
            idle_until(deadline);
    }
    if (g_cpu.master_cycles < deadline) {
        fprintf(stderr, "[sunset] scheduler slice limit: pc=$%06X\n",
                (unsigned)g_resume_pc);
        g_execution_failed = 1;
    }
}

void GameRunOneFrame(void)
{
    uint64_t frame_end, nmi_edge;
    uint32_t remaining;

    if (g_execution_failed)
        return;
    /* The draw callback owns HDMA. Do not consume each table a second time
     * while advancing the beam through CPU execution. */
    g_snes->hdmaBeamOff = true;
    if (!g_resume_pc) {
        g_resume_pc = read_vector(0x00FFFCu);
    }

    snes_sync_master_clock(g_snes, g_cpu.master_cycles);
    remaining = snes_master_clocks_until_line(g_snes, 0);
    frame_end = g_cpu.master_cycles +
        (remaining ? remaining : GAME_MASTER_CYCLES_PER_FRAME);

    remaining = snes_master_clocks_until_line(g_snes, 225);
    nmi_edge = g_cpu.master_cycles + remaining;
    if (nmi_edge < frame_end) {
        run_until(nmi_edge);
        if (g_execution_failed)
            return;
        if (g_snes->nmiEnabled) {
            g_snes->inNmi = true;
            enter_interrupt(g_cpu.emulation ? 0x00FFFAu : 0x00FFEAu);
        }
    }
    run_until(frame_end);
}

void GameDrawPpuFrame(void)
{
    SimpleHdma hdma_chans[8];
    Dma *dma = g_snes->dma;
    int line, ch;

    dma_startDma(dma, g_snesrecomp_last_hdmaen, true);
    for (ch = 0; ch < 8; ++ch)
        SimpleHdma_Init(&hdma_chans[ch], &dma->channel[ch]);
    for (line = 0; line <= 224; ++line) {
        for (ch = 0; ch < 8; ++ch)
            SimpleHdma_DoLine(&hdma_chans[ch]);
        /* CPU beam timing already delivers IRQs. Drawing must not inject a
         * second interrupt into the suspended guest instruction stream. */
        ppu_runLine(g_ppu, line);
    }
}

const RtlGameInfo kGameInfo = {
    .title = "sunsetriders",
    .initialize = NULL,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
};

void GameSessionReset(void)
{
    g_resume_pc = 0;
    g_waiting = 0;
    g_execution_failed = 0;
    interp_bridge_set_master_deadline(0);
}
