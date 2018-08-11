// license:BSD-3-Clause
// copyright-holders:AJR
/*******************************************************************************

    Skeleton driver for Textel Compact portable digital teletype machine.

*******************************************************************************/

#include "emu.h"
#include "cpu/m6502/m65sc02.h"
#include "machine/6522via.h"
#include "machine/mos6551.h"
//#include "machine/msm58321.h"
//#include "video/sed1330.h"


class textelcomp_state : public driver_device
{
public:
	textelcomp_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_chargen(*this, "chargen")
	{ }

	void textelcomp(machine_config &config);

private:
	void mem_map(address_map &map);
	required_device<cpu_device> m_maincpu;
	required_region_ptr<u8> m_chargen;
};


void textelcomp_state::mem_map(address_map &map)
{
	map(0x0000, 0x1eff).ram(); // MB8464A-10L (battery backed?)
	map(0x1f00, 0x1f0f).rw("via0", FUNC(via6522_device::read), FUNC(via6522_device::write));
	map(0x1f10, 0x1f1f).rw("via1", FUNC(via6522_device::read), FUNC(via6522_device::write));
	map(0x1f20, 0x1f2f).rw("via2", FUNC(via6522_device::read), FUNC(via6522_device::write));
	map(0x1f30, 0x1f3f).rw("via3", FUNC(via6522_device::read), FUNC(via6522_device::write));
	map(0x1f40, 0x1f43).rw("acia", FUNC(mos6551_device::read), FUNC(mos6551_device::write));
	map(0x1f70, 0x1f70).noprw();//rw("lcdc", FUNC(sed1330_device::status_r), FUNC(sed1330_device::data_w));
	map(0x1f71, 0x1f71).noprw();//rw("lcdc", FUNC(sed1330_device::data_r), FUNC(sed1330_device::command_w));
	map(0x4000, 0x7fff).ram(); // HY65226ALP-10 (battery backed?)
	map(0x8000, 0x9fff).ram(); // MB8464A-10L (battery backed?)
	map(0xa000, 0xffff).rom().region("maincpu", 0x2000);
}


static INPUT_PORTS_START(textelcomp)
INPUT_PORTS_END


void textelcomp_state::textelcomp(machine_config &config)
{
	M65SC02(config, m_maincpu, 3.6864_MHz_XTAL / 2); // GS65SC02P-2 (clock not verified)
	m_maincpu->set_addrmap(AS_PROGRAM, &textelcomp_state::mem_map);

	VIA6522(config, "via0", 3.6864_MHz_XTAL / 2); // GS65SC22P-2
	VIA6522(config, "via1", 3.6864_MHz_XTAL / 2); // GS65SC22P-2
	VIA6522(config, "via2", 3.6864_MHz_XTAL / 2); // GS65SC22P-2
	VIA6522(config, "via3", 3.6864_MHz_XTAL / 2); // GS65SC22P-2

	MOS6551(config, "acia", 3.6864_MHz_XTAL / 2).set_xtal(3.6864_MHz_XTAL / 2); // GS65SC51P-2

	//SED1330(config, "lcdc", 6.4_MHz_XTAL); // SED1330F + B&W LCD

	//MSM58321(config, "rtc", 32.768_kHz_XTAL); // RTC58321A
}


ROM_START(a1010)
	ROM_REGION(0x8000, "maincpu", 0)
	ROM_LOAD("d15_31.bin",  0x0000, 0x8000, CRC(5ee1175d) SHA1(87ff6a3d5c64a53b0ab23d54aa343365c44d0407))

	ROM_REGION(0x8000, "chargen", 0)
	ROM_LOAD("chargen.bin", 0x0000, 0x8000, CRC(07daa70e) SHA1(8066a0ac238b06fbeeb99c3a2a8a9e70a27db7a9))
ROM_END


COMP(1993, a1010, 0, 0, textelcomp, textelcomp, textelcomp_state, empty_init, "Humantechnik", "Textel Compact A1010-0", MACHINE_IS_SKELETON)
