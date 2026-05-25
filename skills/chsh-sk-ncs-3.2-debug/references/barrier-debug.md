# Co-processor / VPR Barrier Debugging

Barrier hang diagnosis for sQSPI / MSPI soft peripheral on nRF54L series.
Referenced from [chsh-sk-ncs-3.2-debug Mode D](../SKILL.md#mode-d--co-processor--vpr-barrier-debugging).

## D1. Identify the pattern

VPR barrier hang symptoms:
- Busy-wait spins forever (thread stuck in `__XSBx` macro)
- `h0 != h1` in handshake registers: `h0=N, h1=N-1` means VPR one barrier behind

## D2. Debug sequence

Add instrumentation (temporary, remove before release):
```c
_xsb_timeout_tc = 0;
__CSB(p_qspi->p_reg);
if (_xsb_timeout_tc) {
    printk("CSB timeout tc=%u h0=%u h1=%u\n",
           _xsb_timeout_tc,
           sp_handshake_get(p_qspi->p_reg, 0),
           sp_handshake_get(p_qspi->p_reg, 1));
}
```

## D3. Root cause checklist

- [ ] `__DSB()` called before task trigger? (required for AHB/APB write visibility)
- [ ] VPR still in event loop entry when trigger fires? (add graduated re-trigger)
- [ ] Abort sends `__SSB`? (do NOT — if VPR is stuck, SSB also hangs)
- [ ] Events cleared after abort? (`nrf_qspi2_event_clear` for all DMA events)
- [ ] `transfer_in_progress` reset? (needed or next `nrf_sqspi_xfer` returns BUSY)

## D4. Safe abort pattern

```c
void abort_transfer(void) {
    nrf_qspi2_core_disable(p_reg);          // stop DMA, no barriers needed
    nrf_qspi2_event_clear(p_reg, NRF_QSPI2_EVENT_DMA_DONE);
    nrf_qspi2_event_clear(p_reg, NRF_QSPI2_EVENT_DMA_ABORTED);
    nrf_qspi2_event_clear(p_reg, NRF_QSPI2_EVENT_DMA_DONEJOB);
    p_cb->transfer_in_progress = false;
    p_cb->prepared_pending = false;
    // DO NOT call __SSB — it will also hang
}
```
