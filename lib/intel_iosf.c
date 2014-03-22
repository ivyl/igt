#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include "intel_io.h"
#include "intel_reg.h"

#define TIMEOUT_US 500000

static int vlv_sideband_rw(uint32_t port, uint8_t opcode, uint32_t addr,
			   uint32_t *val)
{
	int timeout = 0;
	uint32_t cmd, devfn, be, bar;
	int is_read = (opcode == PUNIT_OPCODE_REG_READ ||
		       opcode == DPIO_OPCODE_REG_READ);

	bar = 0;
	be = 0xf;
	devfn = 16;

	cmd = (devfn << IOSF_DEVFN_SHIFT) | (opcode << IOSF_OPCODE_SHIFT) |
		(port << IOSF_PORT_SHIFT) | (be << IOSF_BYTE_ENABLES_SHIFT) |
		(bar << IOSF_BAR_SHIFT);

	if (intel_register_read(VLV_IOSF_DOORBELL_REQ) & IOSF_SB_BUSY) {
		fprintf(stderr, "warning: pcode (%s) mailbox access failed\n",
			is_read ? "read" : "write");
		return -EAGAIN;
	}

	intel_register_write(VLV_IOSF_ADDR, addr);
	if (!is_read)
		intel_register_write(VLV_IOSF_DATA, *val);

	intel_register_write(VLV_IOSF_DOORBELL_REQ, cmd);

	do {
		usleep(1);
		timeout++;
	} while (intel_register_read(VLV_IOSF_DOORBELL_REQ) & IOSF_SB_BUSY &&
		 timeout < TIMEOUT_US);

	if (timeout >= TIMEOUT_US) {
		fprintf(stderr, "timeout waiting for pcode %s (%d) to finish\n",
			is_read ? "read" : "write", addr);
		return -ETIMEDOUT;
	}

	if (is_read)
		*val = intel_register_read(VLV_IOSF_DATA);
	intel_register_write(VLV_IOSF_DATA, 0);

	return 0;
}

int intel_punit_read(uint8_t addr, uint32_t *val)
{
	return vlv_sideband_rw(IOSF_PORT_PUNIT, PUNIT_OPCODE_REG_READ, addr, val);
}

int intel_punit_write(uint8_t addr, uint32_t val)
{
	return vlv_sideband_rw(IOSF_PORT_PUNIT, PUNIT_OPCODE_REG_WRITE, addr, &val);
}

int intel_nc_read(uint8_t addr, uint32_t *val)
{
	return vlv_sideband_rw(IOSF_PORT_NC, PUNIT_OPCODE_REG_READ, addr, val);
}

int intel_nc_write(uint8_t addr, uint32_t val)
{
	return vlv_sideband_rw(IOSF_PORT_NC, PUNIT_OPCODE_REG_WRITE, addr, &val);
}

uint32_t intel_dpio_reg_read(uint32_t reg, int phy)
{
	uint32_t val;

	vlv_sideband_rw(IOSF_PORT_DPIO, DPIO_OPCODE_REG_READ, reg, &val);
	return val;
}

void intel_dpio_reg_write(uint32_t reg, uint32_t val, int phy)
{
	vlv_sideband_rw(IOSF_PORT_DPIO, DPIO_OPCODE_REG_WRITE, reg, &val);
}
