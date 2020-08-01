// license:BSD-3-Clause
// copyright-holders:AJR
/**********************************************************************

    CIT-101 85-key keyboard

    This serial keyboard appears to transmit and receive signals over
    a single bidirectional wire at 4800 bits per second. Unlike the
    VT100 keyboard whose layout it mimics (indicator LEDs included),
    the CIT-101 keyboard transmits standard ASCII codes for most keys,
    and a MCU is used to decode the scans, generate clicks and beeps
    and handle serial communications. The keyboard also transmits a
    few non-ASCII codes with the most significant bit set (and
    modifiers encoded in the next two bits):

        80       (No key pressed)
	82       Set-Up
        83       Return (host may interpret this as CR or CR+LF)
        84       Up
        85       Down
        86       Right
        87       Left
        88       PF1
        89       PF2
        8A       PF3
        8B       PF4
        8C       , (on numeric keypad)
        8D       - (on numeric keypad)
        8E       . (on numeric keypad)
        8F       Enter (on numeric keypad)
        90       0 (on numeric keypad)
        91       1 (on numeric keypad)
        92       2 (on numeric keypad)
        93       3 (on numeric keypad)
        94       4 (on numeric keypad)
        95       5 (on numeric keypad)
        96       6 (on numeric keypad)
        97       7 (on numeric keypad)
        98       8 (on numeric keypad)
        99       9 (on numeric keypad)
        9A       Clr Home
        9B       Break
        9C       No Scroll
        9D       (Keyboard ID: QWERTY layout)
        9E       (Keyboard ID: AZERTY layout)
        9F       (Invalid key)

    Two types of host commands are processed. Commands with bit 0
    set affect the LEDs (TODO: identify which LEDs are which), and
    commands with bit 0 clear are needed to initiate a new scan.

**********************************************************************/

#include "emu.h"
#include "cit101_kbd.h"
#include "speaker.h"

//**************************************************************************
//  LLE KEYBOARD DEVICE
//**************************************************************************

DEFINE_DEVICE_TYPE(CIT101_KEYBOARD, cit101_keyboard_device, "cit101_kbd", "CIT-101 Keyboard")
DEFINE_DEVICE_TYPE(CIT101E_KEYBOARD, cit101e_keyboard_device, "cit101e_kbd", "CIT-101e Keyboard")

cit101_keyboard_device::cit101_keyboard_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, type, tag, owner, clock)
	, m_mcu(*this, "mcu")
	, m_beeper(*this, "beeper")
	, m_keys(*this, "KEYS%d", 0U)
	, m_kbid(*this, "KBID")
	, m_leds(*this, "led%d", 1U)
	, m_txd_callback(*this)
	, m_kbid_enabled(false)
{
}

cit101_keyboard_device::cit101_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: cit101_keyboard_device(mconfig, CIT101_KEYBOARD, tag, owner, clock)
{
}

cit101e_keyboard_device::cit101e_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: cit101_keyboard_device(mconfig, CIT101E_KEYBOARD, tag, owner, clock)
{
}

void cit101_keyboard_device::device_resolve_objects()
{
	m_txd_callback.resolve_safe();
	m_leds.resolve();
}

void cit101_keyboard_device::device_start()
{
	save_item(NAME(m_kbid_enabled));
}

WRITE_LINE_MEMBER(cit101_keyboard_device::write_rxd)
{
	m_mcu->set_input_line(MCS48_INPUT_IRQ, state ? CLEAR_LINE : ASSERT_LINE);
}


void cit101_keyboard_device::p2_w(u8 data)
{
	m_txd_callback(!BIT(data, 7));
	m_beeper->set_state(BIT(data, 5)); // Bit 6 also involved?
}

void cit101_keyboard_device::leds_w(u8 data)
{
	m_kbid_enabled = !BIT(data, 0);
	for (int i = 0; i < 7; i++)
		m_leds[i] = !BIT(data, i + 1);
}

u8 cit101_keyboard_device::keys_r()
{
	u16 strobes = m_mcu->p1_r() | u16(m_mcu->p2_r() & 0x07) << 8;
	u8 ret = 0xff;

	for (int n = 0; n < 11; n++)
		if (!BIT(strobes, 10 - n))
			ret &= m_keys[n]->read();

	if (m_kbid_enabled)
		ret &= m_kbid->read();

	return ret;
}

void cit101_keyboard_device::prog_map(address_map &map)
{
	map(0x000, 0x7ff).rom().region("program", 0);
}

void cit101_keyboard_device::ext_map(address_map &map)
{
	map(0x00, 0x00).mirror(0xff).w(FUNC(cit101_keyboard_device::leds_w));
}


INPUT_PORTS_START(cit101_keyboard)
	PORT_START("SHIFT")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_SHIFT_1) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0xe7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("MODIFIERS")
	PORT_BIT(1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Repeat") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_SHIFT_2) PORT_CODE(KEYCODE_LCONTROL)

	PORT_START("KEYS0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF3") PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF4") PORT_CHAR(UCHAR_MAMEKEY(F4)) PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD)) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD)) PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD)) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(COMMA_PAD)) PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD)) PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD)) PORT_CODE(KEYCODE_ENTER_PAD)

	PORT_START("KEYS1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF1") PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF2") PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD)) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD)) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD)) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD)) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD)) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD)) PORT_CODE(KEYCODE_2_PAD)

	PORT_START("KEYS2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clr Home") PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x3e, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD)) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD)) PORT_CODE(KEYCODE_DEL_PAD)

	PORT_START("KEYS3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("` ~") PORT_CHAR('`') PORT_CHAR('~') PORT_CHAR(0x1e) PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break") PORT_CHAR(UCHAR_MAMEKEY(PAUSE)) PORT_CODE(KEYCODE_END)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Back Space") PORT_CHAR(0x08) PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CHAR(UCHAR_MAMEKEY(DEL)) PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Line Feed") PORT_CHAR(0x0a) PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CHAR('\\') PORT_CHAR('|') PORT_CHAR(0x1c) PORT_CODE(KEYCODE_BACKSLASH)

	PORT_START("KEYS4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('-') PORT_CHAR('_') PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('=') PORT_CHAR('+') PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('[') PORT_CHAR('{') PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CHAR(']') PORT_CHAR('}') PORT_CHAR(0x1d) PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('\'') PORT_CHAR('"') PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CHAR(0x0d) PORT_CODE(KEYCODE_ENTER)

	PORT_START("KEYS5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('9') PORT_CHAR('(') PORT_CODE(KEYCODE_9)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('0') PORT_CHAR(')') PORT_CODE(KEYCODE_0)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(0x0f) PORT_CODE(KEYCODE_O)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR(0x10) PORT_CODE(KEYCODE_P)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR(0x0c) PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(';') PORT_CHAR(':') PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('.') PORT_CHAR('>') PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CHAR('/') PORT_CHAR('?') PORT_CHAR(0x1f) PORT_CODE(KEYCODE_SLASH)

	PORT_START("KEYS6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('7') PORT_CHAR('&') PORT_CODE(KEYCODE_7)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('8') PORT_CHAR('*') PORT_CODE(KEYCODE_8)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(0x15) PORT_CODE(KEYCODE_U)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CHAR('i') PORT_CHAR('I') PORT_CODE(KEYCODE_I)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CHAR('j') PORT_CHAR('J') PORT_CODE(KEYCODE_J)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR(0x0b) PORT_CODE(KEYCODE_K)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CHAR('m') PORT_CHAR('M') PORT_CODE(KEYCODE_M)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(',') PORT_CHAR('<') PORT_CODE(KEYCODE_COMMA)

	PORT_START("KEYS7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('5') PORT_CHAR('%') PORT_CODE(KEYCODE_5)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('6') PORT_CHAR('^') PORT_CODE(KEYCODE_6)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(0x14) PORT_CODE(KEYCODE_T)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CHAR('y') PORT_CHAR('Y') PORT_CHAR(0x19) PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G (Bell)") PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR(0x07) PORT_CODE(KEYCODE_G)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CHAR('h') PORT_CHAR('H') PORT_CODE(KEYCODE_H)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR(0x02) PORT_CODE(KEYCODE_B)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(0x0e) PORT_CODE(KEYCODE_N)

	PORT_START("KEYS8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 # \xc2\xa3") PORT_CHAR('3') PORT_CHAR('#', 0xa3) PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('4') PORT_CHAR('$') PORT_CODE(KEYCODE_4)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CHAR('e') PORT_CHAR('E') PORT_CHAR(0x05) PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR(0x12) PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR(0x04) PORT_CODE(KEYCODE_D)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR(0x06) PORT_CODE(KEYCODE_F)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR(0x03) PORT_CODE(KEYCODE_C)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR(0x16) PORT_CODE(KEYCODE_V)

	PORT_START("KEYS9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('1') PORT_CHAR('!') PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR('2') PORT_CHAR('@') PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR(0x11) PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR(0x17) PORT_CODE(KEYCODE_W)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR(0x01) PORT_CODE(KEYCODE_A)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR(0x13) PORT_CODE(KEYCODE_S)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(0x1a) PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR(0x18) PORT_CODE(KEYCODE_X)

	PORT_START("KEYS10")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Set-Up") PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(0x1b) PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(0x09) PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("No Scroll") PORT_CHAR(UCHAR_MAMEKEY(SCRLOCK)) PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CHAR(' ') PORT_CODE(KEYCODE_SPACE)

	PORT_START("KBID")
	PORT_CONFNAME(0x01, 0x01, "Keyboard Layout")
	PORT_CONFSETTING(0x01, "QWERTY")
	PORT_CONFSETTING(0x00, "AZERTY")
	PORT_BIT(0xfe, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

ioport_constructor cit101_keyboard_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(cit101_keyboard);
}

void cit101_keyboard_device::device_add_mconfig(machine_config &config)
{
	I8035(config, m_mcu, 2.376_MHz_XTAL); // TMP8048P in EA mode (location marked "8048/8035/8748" on original keyboard)
	m_mcu->set_addrmap(AS_PROGRAM, &cit101_keyboard_device::prog_map);
	m_mcu->set_addrmap(AS_IO, &cit101_keyboard_device::ext_map);
	m_mcu->bus_in_cb().set(FUNC(cit101_keyboard_device::keys_r));
	m_mcu->p2_in_cb().set_ioport("SHIFT");
	m_mcu->p2_out_cb().set(FUNC(cit101_keyboard_device::p2_w));
	m_mcu->t0_in_cb().set_ioport("MODIFIERS").bit(0);
	m_mcu->t1_in_cb().set_ioport("MODIFIERS").bit(1);

	SPEAKER(config, "mono").front_center();
	BEEP(config, m_beeper, 786).add_route(ALL_OUTPUTS, "mono", 0.5); // unknown frequency
}


ROM_START(cit101_kbd)
	ROM_REGION(0x800, "program", 0)
	ROM_LOAD("pkb00.ic4", 0x000, 0x800, CRC(f93f9ce1) SHA1(50fe6b5448f8ec330fa09e8b0125b5b09c680859)) // 1st and 2nd halves identical
ROM_END

ROM_START(cit101e_kbd)
	ROM_REGION(0x1000, "program", 0)
	ROM_LOAD("101e-v12b_kbd_rom.ic4", 0x0000, 0x1000, CRC(b9eeac40) SHA1(1bbbe0d422f87c856a10de5a1492297958ff47f6)) // 1st and 2nd halves identical
ROM_END

const tiny_rom_entry *cit101_keyboard_device::device_rom_region() const
{
	return ROM_NAME(cit101_kbd);
}

const tiny_rom_entry *cit101e_keyboard_device::device_rom_region() const
{
	return ROM_NAME(cit101e_kbd);
}
