#include "kvm/ioport.h"

#include <stdlib.h>
#include <stdio.h>

static void dummy_io(struct kvm_cpu *vcpu, u64 addr, u8 *data, u32 len,
		     u8 is_write, void *ptr)
{
}

static bool debug_io_out(struct ioport *ioport, struct kvm_cpu *vcpu, u16 port, void *data, int size)
{
	dummy_io(vcpu, port, data, size, true, NULL);
	return 0;
}

static struct ioport_operations debug_ops = {
	.io_out		= debug_io_out,
};

static void seabios_debug_io(struct kvm_cpu *vcpu, u64 addr, u8 *data,
			     u32 len, u8 is_write, void *ptr)
{
	char ch;

	if (!is_write)
		return;

	ch = ioport__read8(data);

	putchar(ch);
}

static bool seabios_debug_io_out(struct ioport *ioport, struct kvm_cpu *vcpu, u16 port, void *data, int size)
{
	seabios_debug_io(vcpu, port, data, size, true, NULL);
	return 0;
}

static struct ioport_operations seabios_debug_ops = {
	.io_out		= seabios_debug_io_out,
};

static bool dummy_io_in(struct ioport *ioport, struct kvm_cpu *vcpu, u16 port, void *data, int size)
{
	dummy_io(vcpu, port, data, size, false, NULL);
	return true;
}

static bool dummy_io_out(struct ioport *ioport, struct kvm_cpu *vcpu, u16 port, void *data, int size)
{
	dummy_io(vcpu, port, data, size, true, NULL);
	return true;
}

static struct ioport_operations dummy_read_write_ioport_ops = {
	.io_in		= dummy_io_in,
	.io_out		= dummy_io_out,
};

static struct ioport_operations dummy_write_only_ioport_ops = {
	.io_out		= dummy_io_out,
};

/*
 * The "fast A20 gate"
 */

static void ps2_control_io(struct kvm_cpu *vcpu, u64 addr, u8 *data, u32 len,
			   u8 is_write, void *ptr)
{
	/*
	 * A20 is always enabled.
	 */
	if (!is_write)
		ioport__write8(data, 0x02);
}

static bool ps2_control_a_io_in(struct ioport *ioport, struct kvm_cpu *vcpu, u16 port, void *data, int size)
{
	ps2_control_io(vcpu, port, data, size, false, NULL);
	return true;
}

static struct ioport_operations ps2_control_a_ops = {
	.io_in		= ps2_control_a_io_in,
	.io_out		= dummy_io_out,
};

void ioport__map_irq(u8 *irq)
{
}

static int ioport__setup_arch(struct kvm *kvm)
{
	int r;

	/* Legacy ioport setup */

	/* 0000 - 001F - DMA1 controller */
	r = ioport__register(kvm, 0x0000, &dummy_read_write_ioport_ops, 32, NULL);
	if (r < 0)
		return r;

	/* 0x0020 - 0x003F - 8259A PIC 1 */
	r = ioport__register(kvm, 0x0020, &dummy_read_write_ioport_ops, 2, NULL);
	if (r < 0)
		return r;

	/* PORT 0040-005F - PIT - PROGRAMMABLE INTERVAL TIMER (8253, 8254) */
	r = ioport__register(kvm, 0x0040, &dummy_read_write_ioport_ops, 4, NULL);
	if (r < 0)
		return r;

	/* 0092 - PS/2 system control port A */
	r = ioport__register(kvm, 0x0092, &ps2_control_a_ops, 1, NULL);
	if (r < 0)
		return r;

	/* 0x00A0 - 0x00AF - 8259A PIC 2 */
	r = ioport__register(kvm, 0x00A0, &dummy_read_write_ioport_ops, 2, NULL);
	if (r < 0)
		return r;

	/* 00C0 - 001F - DMA2 controller */
	r = ioport__register(kvm, 0x00C0, &dummy_read_write_ioport_ops, 32, NULL);
	if (r < 0)
		return r;

	/* PORT 00E0-00EF are 'motherboard specific' so we use them for our
	   internal debugging purposes.  */
	r = ioport__register(kvm, IOPORT_DBG, &debug_ops, 1, NULL);
	if (r < 0)
		return r;

	/* PORT 00ED - DUMMY PORT FOR DELAY??? */
	r = ioport__register(kvm, 0x00ED, &dummy_write_only_ioport_ops, 1, NULL);
	if (r < 0)
		return r;

	/* 0x00F0 - 0x00FF - Math co-processor */
	r = ioport__register(kvm, 0x00F0, &dummy_write_only_ioport_ops, 2, NULL);
	if (r < 0)
		return r;

	/* PORT 0278-027A - PARALLEL PRINTER PORT (usually LPT1, sometimes LPT2) */
	r = ioport__register(kvm, 0x0278, &dummy_read_write_ioport_ops, 3, NULL);
	if (r < 0)
		return r;

	/* PORT 0378-037A - PARALLEL PRINTER PORT (usually LPT2, sometimes LPT3) */
	r = ioport__register(kvm, 0x0378, &dummy_read_write_ioport_ops, 3, NULL);
	if (r < 0)
		return r;

	/* PORT 03D4-03D5 - COLOR VIDEO - CRT CONTROL REGISTERS */
	r = ioport__register(kvm, 0x03D4, &dummy_read_write_ioport_ops, 1, NULL);
	if (r < 0)
		return r;
	r = ioport__register(kvm, 0x03D5, &dummy_write_only_ioport_ops, 1, NULL);
	if (r < 0)
		return r;

	r = ioport__register(kvm, 0x402, &seabios_debug_ops, 1, NULL);
	if (r < 0)
		return r;

	/* 0510 - QEMU BIOS configuration register */
	r = ioport__register(kvm, 0x510, &dummy_read_write_ioport_ops, 2, NULL);
	if (r < 0)
		return r;

	return 0;
}
dev_base_init(ioport__setup_arch);
