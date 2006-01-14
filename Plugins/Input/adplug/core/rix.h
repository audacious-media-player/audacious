/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * rix.h - Dayu OPL Format Player by palxex <palxex@163.com/palxex.ys168.com> 
 */

#include "player.h"

class CrixPlayer: public CPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CrixPlayer(Copl *newopl)
		: CPlayer(newopl),I(0),T(0),mus_block(0),ins_block(0),rhythm(0),mutex(0),music_on(0),
		  pause_flag(0),band(0),band_low(0),e0_reg_flag(0),bd_modify(0),sustain(0),dro_end(0)
	{
	//	memset(buffer,0,97948);
		int i=0,j=i*3;
		unsigned long t=9;

		if(opl->gettype() == Copl::TYPE_OPL2)
		  opl3_mode = 0;
		else
		  opl3_mode = 1;
	};
	~CrixPlayer()
	{};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);
	float getrefresh();

	std::string gettype()
	{ return std::string("Softstar RIX OPL Music Format"); };

	typedef struct {unsigned char v[14];}ADDT;

protected:	
	unsigned char dro[64000];
	unsigned char buf_addr[655360];  /* rix files' buffer */
	unsigned short buffer[300];
	unsigned short a0b0_data2[11];
	unsigned char a0b0_data3[18];
	unsigned char a0b0_data4[18];
	unsigned char a0b0_data5[96];
	unsigned char addrs_head[96];
	unsigned short insbuf[28];
	unsigned short displace[11];
	ADDT reg_bufs[18];
	unsigned long pos,length;
	unsigned long msdone,mstotal;
	unsigned short delay;
	unsigned char index, opl3_mode;
	enum OplMode {
		ModeOPL2,ModeOPL3,ModeDUALOPL2
	} mode;

	static const unsigned char adflag[18];
	static const unsigned char reg_data[18];
	static const unsigned char ad_C0_offs[18];
	static const unsigned char modify[28];
	static const unsigned char bd_reg_data[124];
	static unsigned char for40reg[18];
	static unsigned short mus_time;
	unsigned int I,T;
	unsigned short mus_block;
	unsigned short ins_block;
	unsigned char rhythm;
	unsigned char mutex;
	unsigned char music_on;
	unsigned char pause_flag;
	unsigned short band;
	unsigned char band_low;
	unsigned short e0_reg_flag;
	unsigned char bd_modify;
	int sustain;
	int dro_end;

	#define ad_08_reg() ad_bop(8,0)    /**/
	inline void ad_20_reg(unsigned short);              /**/
	inline void ad_40_reg(unsigned short);              /**/
	inline void ad_60_reg(unsigned short);              /**/
	inline void ad_80_reg(unsigned short);              /**/
	inline void ad_a0b0_reg(unsigned short);            /**/
	inline void ad_a0b0l_reg(unsigned short,unsigned short,unsigned short); /**/
	inline void ad_a0b0l_reg_(unsigned short,unsigned short,unsigned short); /**/
	inline void ad_bd_reg();                  /**/
	inline void ad_bop(unsigned short,unsigned short);                     /**/
	inline void ad_C0_reg(unsigned short);              /**/
	inline void ad_E0_reg(unsigned short);              /**/
	inline unsigned short ad_initial();                 /**/
	inline unsigned short ad_test();                    /**/
	inline void crc_trans(unsigned short,unsigned short);         /**/
	inline void data_initial();               /* done */
	inline void init();                       /**/
	inline void ins_to_reg(unsigned short,unsigned short*,unsigned short);  /**/
	inline void int_08h_entry();    /**/
	inline void music_ctrl();                 /**/
	inline void Pause();                      /**/
	inline void prep_int();                   /**/
	inline void prepare_a0b0(unsigned short,unsigned short);      /**/
	inline void rix_90_pro(unsigned short);             /**/
	inline void rix_A0_pro(unsigned short,unsigned short);        /**/
	inline void rix_B0_pro(unsigned short,unsigned short);        /**/
	inline void rix_C0_pro(unsigned short,unsigned short);        /**/
	inline void rix_get_ins();                /**/
	inline unsigned short rix_proc();                   /**/
	inline void set_new_int();
	inline void set_speed(unsigned short);              /**/
	inline void set_time(unsigned short);               /**/
	inline void switch_ad_bd(unsigned short);           /**/
	inline unsigned int strm_and_fr(unsigned short);               /* done */
};
