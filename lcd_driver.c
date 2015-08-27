/*
*
*  Under GPL License
*
*/

#include "lcd_driver.h"

#include "stm32f10x_fsmc.h"

static void delay(__IO u32 nCount){
	for(; nCount != 0; nCount--);
}

static void delay_ms(__IO u32 nCount){
	for(; nCount != 0; nCount--)
		delay(SystemCoreClock/1000/6);
}


static GraphicContext _d_g_ctx = {
	.direction = 0,
	.invert = 0
};

void LCD_SetEntryMode(u8 direction, u8 inv){
	u16 reg = 0x1000;
	u8 LUT[] = {0b110, 0b011, 0b000, 0b101, 0b100, 0b111, 0b010, 0b111};
	direction &=3;
	inv = (inv>0)&1;
	_d_g_ctx.direction = direction;
	_d_g_ctx.invert = inv;
	LCD_WR_CMD(0x0003, reg | (LUT[direction << inv] << 3));
}

void LCD_SetPoint(u16 x, u16 y){
	LCD_WR_CMD(0x20, x);
	LCD_WR_CMD(0x21, y);
	LCD_WR_REG(0x22);
}

void LCD_SetPoint_InCtx(u16 x, u16 y){
	switch (_d_g_ctx.direction){
	case 1:
		LCD_WR_CMD(0x20, y);
		LCD_WR_CMD(0x21, LCD_SCR_HEI1 - x);
		break;
	case 2:
		LCD_WR_CMD(0x20, LCD_SCR_WID1 - x);
		LCD_WR_CMD(0x21, LCD_SCR_HEI1 - y);
		break;
	case 3:
		LCD_WR_CMD(0x20, LCD_SCR_WID1 - y);
		LCD_WR_CMD(0x21, x);
		break;
	case 0:
		LCD_WR_CMD(0x20, x);
		LCD_WR_CMD(0x21, y);
	}
	LCD_WR_REG(0x22);
}

void LCD_SetWindow(u16 left, u16 top, u16 width, u16 height){
	u16 _left, _top, _width, _height, _x, _y;

	switch (_d_g_ctx.direction){
	case 1:
		_left = top;
		_top = LCD_SCR_HEI - left - width;
		_width = height;_height = width;
		_x = top; _y = LCD_SCR_HEI1 - left;
		break;
	case 2:
		_left = LCD_SCR_WID - left - width;
		_top = LCD_SCR_HEI - top - height;
		_width = width;	_height = height;
		_x = LCD_SCR_WID1 - left; _y = LCD_SCR_HEI1 - top;
		break;
	case 3:
		_left = LCD_SCR_WID - top - height;
		_top = left;
		_width = height;_height = width;
		_x = LCD_SCR_WID1 - top; _y = left;
		break;
	default:
		_left = left; _top = top; _width = width; _height = height;
		_x = left; _y = top;
	}

	LCD_WR_CMD(0x50, _left);
	LCD_WR_CMD(0x51, _left+_width-1);
	LCD_WR_CMD(0x52, _top);
	LCD_WR_CMD(0x53, _top+_height-1);
	LCD_SetPoint(_x, _y);
}


//static u32 linear_x155(u32 a, u32 b, u8 u){ // u= u+ 0.5 max=15.5/16
//	return (a*u + b*(16-u) + ((a>>1)&0x70707) - ((b>>1)&0x70707));
//}

static u32 linear_x16(u32 a, u32 b, u8 u){ // max=16
	return a*u + b*(16-u);
}

static u32 linear_x32(u32 a, u32 b, u8 u){ // max=32
	return a*u + b*(32-u);
}


inline u16 LCD_GetPixel(){
	u16 c565;
	c565 = LCD_RD_DAT1();
	LCD_WR_DAT(c565);
	return c565;
}

inline void LCD_PutPixel(u16 c565){
	LCD_WR_DAT(c565);
}

void LCD_BlendPixel_x16(const u32 fc888, const u8 a4){
	u16 c565;
	u32 c888;
	c565 = LCD_RD_DAT1();
	c888 = C_RGB565to888h4(c565);
	c888 = linear_x16(fc888, c888, a4);
	c565 = C_RGB888to565(c888);
	LCD_WR_DAT(c565);
}

void LCD_BlendPixel_x32(const u32 fcaba, const u8 a5){
	u16 c565;
	u32 caba;
	c565 = LCD_RD_DAT1();
	caba = C_RGB565toABAh5(c565);
	caba = linear_x32(fcaba, caba, a5);
	c565 = C_RGBABAto565(caba);
	LCD_WR_DAT(c565);
}

const u8 LUT15to32[16]={
	0,  2,  4,  6,  9, 11, 13, 15, 17, 19, 21, 23, 26, 28, 30, 32
};
const u8 LUT225to32[226]={
	0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,
	2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  5,  5,
	5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,
	7,  7,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9, 10,
   10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12,
   12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14,
   15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17,
   17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19,
   19, 19, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 22,
   22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24,
   24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26,
   27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29,
   29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31,
   31, 32, 32, 32, 32
};
const u8 LUT255to32[256]={
	0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
	2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,
	4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,
	6,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,
	9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 11,
	11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13,
	13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15,
	15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17,
	17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19,
	19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21,
	21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23,
	23, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25,
	26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 28,
	28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30,
	30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 32, 32, 32,
	32
};

u8 LCD_ScaleAlpha_32(u8 v, u8 m){
	switch (m){
	case 15:return LUT15to32[v];
	case 225:return LUT225to32[v];
	case 255:return LUT255to32[v];
	default:return (u16)(v)*32/m;
	}
}

void LCD_GetImage_RGB565(u16 *buf, u32 size){
	u16 *p = buf;
	while (size--){
		*p = LCD_RD_DAT1();
		LCD_WR_DAT(*p++);
	}
}

void LCD_PutImage_RGB565(const u16 *buf, u32 size){
	u16 *p = (u16 *)buf;
	while (size--){
		LCD_WR_DAT(*p++);
	}
}

void LCD_PutImage_RGB4444(const u16 *buf, u32 size){
	u16 *p = (u16 *)buf;
	u16 t0;
	u32 fc0;
	while (size--){
		t0 = *p++;
		fc0 = C_RGB4444toABAh5(t0);
		LCD_BlendPixel_x32(fc0, LUT15to32[C_ALPHA4(t0)]);
	}
}

void LCD_PutChar_RGB565(const u8* glyph, u16 size, u16 fc, u8 a8){
	u8 *p = (u8 *)glyph;
	u8 t0;
	u32 fc0;
	fc0 = C_RGB565toABAh5(fc);
	while (size--){
		t0 = *p++;
		LCD_BlendPixel_x32(fc0, (u16)(a8)*(t0&0xf)/118);
		LCD_BlendPixel_x32(fc0, (u16)(a8)*(t0>>4) /118);
	}
}

void LCD_PutChar_RGB4444(const u8* glyph, u16 size, u16 fc){
	u8 *p = (u8 *)glyph;
	u8 t0, fa0;
	u32 fc0;
	fa0 = C_ALPHA4(fc);
	fc0 = C_RGB4444toABAh5(fc);
	while (size--){
		t0 = *p++;
		LCD_BlendPixel_x32(fc0, LUT225to32[fa0*(t0&0xf)]);
		LCD_BlendPixel_x32(fc0, LUT225to32[fa0*(t0>>4) ]);
	}
}

void LCD_BlendImage_RGB565(const u16 *buf, u32 size, u8 a8){
	u16 *p = (u16 *)buf;
	u16 t0;
	u32 fc0;
	a8 = LUT255to32[a8];
	while (size--){
		t0 = *p++;
		fc0 = C_RGB565toABAh5(t0);
		LCD_BlendPixel_x32(fc0, a8);
	}
}

void LCD_BlendImage_RGB4444(const u16 *buf, u32 size, u8 a8){
	u16 *p = (u16 *)buf;
	u16 t0;
	u32 fc0;
	while (size--){
		t0 = *p++;
		fc0 = C_RGB4444toABAh5(t0);
		LCD_BlendPixel_x32(fc0, (u32)(a8)*C_ALPHA4(t0)/118);
	}
}

void LCD_MaskImage_RGB565(const u16 *buf, u32 size, const u8* a8){
	u16 *p = (u16 *)buf;
	u8 *q = (u8 *)a8;
	u16 t0;
	u32 fc0;
	while (size--){
		t0 = *p++;
		fc0 = C_RGB565toABAh5(t0);
		LCD_BlendPixel_x32(fc0, LUT255to32[*q++]);
	}
}

void LCD_MaskImage_RGB4444(const u16 *buf, u32 size, const u8* a8){
	u16 *p = (u16 *)buf;
	u8 *q = (u8 *)a8;
	u16 t0;
	u32 fc0;
	while (size--){
		t0 = *p++;
		fc0 = C_RGB4444toABAh5(t0);
		LCD_BlendPixel_x32(fc0, LUT255to32[*q++]);
	}
}

void LCD_BitMaskImage_RGB565(const u16 *buf, u32 size, const bitmask mask){
	u16 *p = (u16 *)buf;
	u8 *q = (bitmask)mask, t1 = 0, cnt;
	u16 t0;
	cnt = 0;
	while (size--){
		if (cnt == 0){
			t1 = *q++;
			cnt = 8;
		}
		t0 = (t1&1)?(*p):LCD_RD_DAT1();
		LCD_WR_DAT(t0);
		p++;
		t1 >>= 1;
		cnt--;
	}
}

void LCD_BitMaskImage_RGB4444(const u16 *buf, u32 size, const bitmask mask){
	u16 *p = (u16 *)buf;
	u8 *q = (bitmask)mask, t1 = 0, cnt;
	u16 t0;
	u32 fc0;
	cnt = 0;
	while (size--){
		if (cnt == 0){
			t1 = *q++;
			cnt = 8;
		}
		if (t1 & 1){
			t0 = *p;
			fc0 = C_RGB4444toABAh5(t0);
			LCD_BlendPixel_x32(fc0, LUT15to32[C_ALPHA4(t0)]);
		}
		else {
			t0 = LCD_RD_DAT1();
			LCD_WR_DAT(t0);
		}
		p++;
		t1 >>= 1;
		cnt--;
	}
}


u8 LCD_GetBitMask(bitmask mask, u16 x, u16 y, u16 w){
	u32 t;
	t = y*w+x;
	y = t & 0x7;
	return (mask[t>>3]>>y) & 1;
}

void LCD_ResetBitMask(bitmask mask, u16 x, u16 y, u16 w){
	u32 t;
	t = y*w+x;
	y = t & 0x7;
	mask[t>>3] &= ~(1<<y);
}

void LCD_SetBitMask(bitmask mask, u16 x, u16 y, u16 w){
	u32 t;
	t = y*w+x;
	y = t & 0x7;
	mask[t>>3] |= 1<<y;
}

void LCD_DrawLineBody(DrawLineContext ctx){
	static u16 t;
	static u32 c;
	static s16 x, y;
	static s32 dl;
	for (y=ctx.ly0;y<=ctx.ly1;y++){
		for (x=ctx.lx0;x<=ctx.lx1;x++){
			t = LCD_RD_DAT1();
			if (!LCD_GetBitMask(ctx.bm, ctx.bmx+x, ctx.bmy+y, ctx.bmw)){
				dl = (ctx.rx*y-ctx.ry*x);
				dl = dl * dl;
				dl = (dl + (ctx.ll>>1))/ ctx.ll;

				if (ctx.llhw1>dl){
					c = C_RGB565toABAh5(t);
					if (dl<=ctx.llhw) {
						c = linear_x32(ctx.fc, c, ctx.alpha32);
						LCD_SetBitMask(ctx.bm, ctx.bmx+x, ctx.bmy+y, ctx.bmw);
					}
					else c = linear_x32(ctx.fc, c, ctx.alpha32*(ctx.llhw1-dl)/ctx.grad);
					t = C_RGBABAto565(c);
				}
			}
			LCD_WR_DAT(t);
		}
	}
}

void LCD_DrawLineEndPart(DrawLineContext ctx){
	static u16 t;
	static u32 c;
	static s16 x, y;
	static s32 dl;
	static u32 yy0, yy1, dd0, dd1;
	for (y=ctx.ly0;y<=ctx.ly1;y++){
		yy0 = y*y;
		yy1 = (y-ctx.ry)*(y-ctx.ry);
		for (x=ctx.lx0;x<=ctx.lx1;x++){
			t = LCD_RD_DAT1();
			if (!LCD_GetBitMask(ctx.bm, ctx.bmx+x, ctx.bmy+y, ctx.bmw)){
//			if (1){
				dl = (ctx.rx*y-ctx.ry*x);
				dl = dl * dl;
				dl = (dl + (ctx.ll>>1))/ ctx.ll;
				if (ctx.llhw1>dl){
					dd0 = yy0 + x*x;
					dd1 = yy1 + (x-ctx.rx)*(x-ctx.rx);
					dl = (dd0+ctx.ll<dd1)?dd0:(dd1+ctx.ll<dd0)?dd1:dl;
					if (ctx.llhw1>dl){
						c = C_RGB565toABAh5(t);
						if (dl<=ctx.llhw) {
							c = linear_x32(ctx.fc, c, ctx.alpha32);
							LCD_SetBitMask(ctx.bm, ctx.bmx+x, ctx.bmy+y, ctx.bmw);
						}
						else c = linear_x32(ctx.fc, c, ctx.alpha32*(ctx.llhw1-dl)/ctx.grad);
						t = C_RGBABAto565(c);
					}
				}
			}
			LCD_WR_DAT(t);
		}
	}
}


void LCD_DrawLineEnd(DrawLineContext ctx, u16 sx, u16 sy, u16 ex, u16 ey, u16 lh){
	u16 xMin, xMax, yMin, yMax;

	/* U: (sy<ey)||(sy==ey)&&(sx<ex)
	 * R: (sx>ex)||(sx==ex)&&(sy<ey)
	 * D: (sy>ey)||(sy==ey)&&(sx>ex)
	 * L: (sx<ex)||(sx==ex)&&(sy>ey)
	 */

	xMin = min(sx, ex);xMax = max(sx, ex);
	yMin = min(sy, ey);yMax = max(sy, ey);

	if ((sy<ey)||((sy==ey)&&(sx<ex))){
		ctx.lx0 = xMin-lh-sx;
		ctx.lx1 = xMax-sx;
		ctx.ly0 = yMin-lh-sy;
		ctx.ly1 = yMin-1-sy;
		LCD_SetWindow(xMin-lh, yMin-lh, xMax-xMin+lh+1, lh);
		LCD_DrawLineEndPart(ctx);
	}

	if ((sx>ex)||((sx==ex)&&(sy<ey))){
		ctx.lx0 = xMax+1-sx;
		ctx.lx1 = xMax+lh-sx;
		ctx.ly0 = yMin-lh-sy;
		ctx.ly1 = yMax-sy;
		LCD_SetWindow(xMax+1, yMin-lh, lh, yMax-yMin+lh+1);
		LCD_DrawLineEndPart(ctx);
	}

	if ((sy>ey)||((sy==ey)&&(sx>ex))){
		ctx.lx0 = xMin-sx;
		ctx.lx1 = xMax+lh-sx;
		ctx.ly0 = yMax+1-sy;
		ctx.ly1 = yMax+lh-sy;
		LCD_SetWindow(xMin, yMax+1, xMax-xMin+lh+1, lh);
		LCD_DrawLineEndPart(ctx);
	}

	if ((sx<ex)||((sx==ex)&&(sy>ey))){
		ctx.lx0 = xMin-lh-sx;
		ctx.lx1 = xMin-1-sx;
		ctx.ly0 = yMin-sy;
		ctx.ly1 = yMax+lh-sy;
		LCD_SetWindow(xMin-lh, yMin, lh, yMax-yMin+lh+1);
		LCD_DrawLineEndPart(ctx);
	}
}

bitmask LCD_DrawCircle(u16 cx, u16 cy, u16 r, u16 fc, u16 lw
					   , bitmask bm, s16 bmx, s16 bmy, u16 bmw){
	s16 x, y;
	u16 ge, gi, c565;
	u32 rre, rri, rre1, rri1, dd, yy;
	u32 caba, fc0 = C_RGB4444toABAh5(fc);
	u8 a = C_ALPHA4(fc), a0;
	lw=(lw>0)?lw:1;
	rre = (r+lw)*(r+lw); rri = (r>lw)?(r-lw)*(r-lw):0;
	rre1 = (r+lw+1)*(r+lw+1); rri1 = (r>lw+1)?(r-lw-1)*(r-lw-1):0;
	ge = (r+lw)*2+1; gi = (r>lw)?(r-lw)*2-1:1;
	LCD_SetWindow(cx - r - lw, cy - r - lw, (r+lw)*2+1, (r+lw)*2+1);
	for (y=-r-lw;y<=r+lw;y++){
		yy = y*y;
		for (x=-r-lw;x<=r+lw;x++){
			dd = x*x+yy;
			c565 = LCD_RD_DAT1();
			if ((rri1<dd) && (rre1>dd)){
				if (dd>=rri){
					if (dd<=rre){
						a0 = LUT15to32[a];
						LCD_SetBitMask(bm, bmx+x, bmy+y, bmw);
					}
					else a0 = a*(rre1-dd)*2/ge;
				}
				else a0 = a*(dd-rri1)*2/gi;
				caba = C_RGB565toABAh5(c565);
				caba = linear_x32(fc0, caba, a0);
				c565 = C_RGBABAto565(caba);
			}
			LCD_WR_DAT(c565);
		}
	}
	return bm;
}


void LCD_FillRectangle_RGB565(u16 left, u16 top, u16 w, u16 h, u16 fc){
	u16 x, y;
	LCD_SetWindow(left, top, w, h);
	for (y=0;y<h;y++){
		for (x=0;x<w;x++){
			LCD_WR_DAT(fc);
		}
	}
}

void LCD_FillRectangle_RGB4444(u16 left, u16 top, u16 w, u16 h, u16 fc){
	u16 x, y;
	u32 fc0 = C_RGB4444toABAh5(fc);
	u8 a = C_ALPHA4(fc);
	LCD_SetWindow(left, top, w, h);
	for (y=0;y<h;y++){
		for (x=0;x<w;x++){
			LCD_BlendPixel_x32(fc0, LUT15to32[a]);
		}
	}
}


void LCD_FillCircle_RGB565(u16 cx, u16 cy, u16 r, u16 fc){
	s16 x, y;
	u16 g, c565;
	u32 rr, rr1, dd, yy;
	u32 caba, fc0 = C_RGB565toABAh5(fc);
	rr = r*r; rr1 = (r+1)*(r+1); g = r+r+1;
	LCD_SetWindow(cx - r, cy - r, r*2+1, r*2+1);
	for (y=-r;y<=r;y++){
		yy = y*y;
		for (x=-r;x<=r;x++){
			dd = x*x+yy;
			if (dd<=rr) {
				c565 = fc;
			}
			else {
				c565 = LCD_RD_DAT1();
				if (rr1>dd){
					caba = C_RGB565toABAh5(c565);
					caba = linear_x32(fc0, caba, (rr1-dd)*32/g);
					c565 = C_RGBABAto565(caba);
				}
			}
			LCD_WR_DAT(c565);
		}
	}
}

void LCD_FillCircle_RGB4444(u16 cx, u16 cy, u16 r, u16 fc){
	s16 x, y;
	u16 g, c565;
	u32 rr, rr1, dd, yy;
	u32 caba, fc0 = C_RGB4444toABAh5(fc);
	u8 a = C_ALPHA4(fc), a0;
	rr = r*r; rr1 = (r+1)*(r+1); g = r+r+1;
	LCD_SetWindow(cx - r, cy - r, r*2+1, r*2+1);
	for (y=-r;y<=r;y++){
		yy = y*y;
		for (x=-r;x<=r;x++){
			dd = x*x+yy;
			c565 = LCD_RD_DAT1();
			if (rr1>dd){
				a0 = (dd<=rr)?LUT15to32[a]:(a*(rr1-dd)*2/g);
				caba = C_RGB565toABAh5(c565);
				caba = linear_x32(fc0, caba, a0);
				c565 = C_RGBABAto565(caba);
			}
			LCD_WR_DAT(c565);
		}
	}
}

bitmask LCD_Fill_Floodfill4_Core(u16 left, u16 top, u16 right, u16 bottom
							   , u16 sx, u16 sy, s16 mx, s16 my
							   , bitmask mask, u16 bmw, u16 fc, u16 qlen
							   , s16 *qx, s16* qy){

	u16 fc0 = C_RGB4444toABAh5(fc);
	u8 a0 = LUT15to32[C_ALPHA4(fc)];

	u16 head, tail;
	s16 cx = 0, cy = 0;
	head = tail = 0;

	#define PUSH(x, y) ({if(_mod(tail+1,qlen)!=head){\
						LCD_SetBitMask(mask, mx + x, my + y, bmw);\
						qx[tail]=x;qy[tail]=y;tail++;if(tail==qlen)tail=0;\
						}})
	#define POP ({cx=qx[head];cy=qy[head];head++;if(head==qlen)head=0;})

	LCD_SetWindow(left, top, right - left + 1, bottom - top + 1);
	PUSH(cx, cy);
	while (head != tail){
		POP;
		LCD_SetPoint_InCtx(sx + cx, sy + cy);
		LCD_BlendPixel_x32(fc0, a0);
		if ((sx+cx+1<=right) && (!LCD_GetBitMask(mask, mx + cx + 1, my + cy, bmw))) PUSH(cx+1, cy);
		if ((sx+cx>=left+1)  && (!LCD_GetBitMask(mask, mx + cx - 1, my + cy, bmw))) PUSH(cx-1, cy);
		if ((sx+cx+1<=bottom)&& (!LCD_GetBitMask(mask, mx + cx, my + cy + 1, bmw))) PUSH(cx, cy+1);
		if ((sx+cx>=top+1)   && (!LCD_GetBitMask(mask, mx + cx, my + cy - 1, bmw))) PUSH(cx, cy-1);
	}

	return mask;
}


void LCD_Fill_BitMaskShadow(u16 left, u16 top, u16 right, u16 bottom
							, bitmask mask, s16 mx, s16 my, u16 bmw
							, u16 sc, s16 sx, s16 sy, s16 step5){

	u32 sc0 = C_RGB4444toABAh5(sc), caba;
	u8 a0 = C_ALPHA4(sc), inv;
	u16 x, y, w = right - left + 1, h = bottom - top + 1;
	u16 i, sum, c565, step5h;

	if(step5 == 0) step5 = abs(sx)+abs(sy);
	inv = step5<0;
	step5 = abs(step5);
	update_min(step5, 32);
	step5h = step5>>1;

	LCD_SetWindow(left, top, w, h);
	for (y=0;y<h;y++) for (x=0;x<w;x++){
		c565 = LCD_RD_DAT1();
		if (!LCD_GetBitMask(mask, mx + x, my + y, bmw)){
			sum = 0;
			for (i=1;i<=step5;i++){
				sum += LCD_GetBitMask(mask, mx + x - (sx*i+step5h)/step5, my + y - (sy*i+step5h)/step5, bmw);
			}
			if (sum>0){
				sum = a0*sum*2/step5;
				update_min(sum, 32);
				if (inv) sum = 32 - sum;
				caba = C_RGB565toABAh5(c565);
				caba = linear_x32(sc0, caba, sum);
				c565 = C_RGBABAto565(caba);
			}
		}
		LCD_WR_DAT(c565);
	}

}











void LCD_Cmd_InitFSMC(){
	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  p;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |
		RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE , ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	//LCD Rest
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	// FSMC-D0--D15
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 |
		GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |
		GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//FSMC NE1  LCD CS
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	//FSMC RS
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 ;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//FSMC Spec
	p.FSMC_AddressSetupTime = 0x02;
	p.FSMC_AddressHoldTime = 0x00;
	p.FSMC_DataSetupTime = 0x05;
	p.FSMC_BusTurnAroundDuration = 0x00;
	p.FSMC_CLKDivision = 0x00;
	p.FSMC_DataLatency = 0x00;
	p.FSMC_AccessMode = FSMC_AccessMode_B;

	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_NOR;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	/* Enable FSMC Bank1_SRAM Bank */
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

void LCD_Cmd_InitBacklight(){
	GPIO_InitTypeDef GPIO_InitStructure;
//	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
//	TIM_OCInitTypeDef  TIM_OCInitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_13);
}

void LCD_Cmd_NReset(){
	GPIO_SetBits(GPIOE, GPIO_Pin_1);
	delay_ms(1);
	GPIO_ResetBits(GPIOE, GPIO_Pin_1);
	delay_ms(10);
	GPIO_SetBits(GPIOE, GPIO_Pin_1);
	delay_ms(50);
}

void LCD_Cmd_Init(void)
{
	LCD_Cmd_NReset();
	assert_param(LCD_RD_REG(0) == 0x9325);

	//************* Start Initial Sequence **********//
	LCD_WR_CMD(0x0001, 0x0100); // set SS and SM bit
	LCD_WR_CMD(0x0002, 0x0700); // set 1 line inversion
	LCD_SetEntryMode(0, 0);
	LCD_WR_CMD(0x0004, 0x0000); // Resize register
//	LCD_WR_CMD(0x0007, 0x3033); // Screen ON, partial only
	LCD_WR_CMD(0x0007, 0x0133); // Screen ON, no partial
//	LCD_WR_CMD(0x0008, 0x0207); // set the back porch and front porch
//	LCD_WR_CMD(0x0009, 0x032f); // set non-display area refresh cycle ISC[3:0]
//	LCD_WR_CMD(0x000A, 0x0000); // FMARK function
	LCD_WR_CMD(0x000C, 0x3001); // System interface - 16bit
//	LCD_WR_CMD(0x000D, 0x0000); // Frame marker Position
//	LCD_WR_CMD(0x000F, 0x0000); // RGB interface polarity
	//*************Power On sequence ****************//
	LCD_WR_CMD(0x0010, 0x0000); // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0011, 0x0007); // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_WR_CMD(0x0012, 0x0000); // VREG1OUT voltage
	LCD_WR_CMD(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude
//	LCD_WR_CMD(0x0007, 0x0001); // Display OFF
	delay_ms(200); // Dis-charge capacitor power voltage
	LCD_WR_CMD(0x0010, 0x1490); // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0011, 0x0227); // DC1[2:0], DC0[2:0], VC[2:0]
	delay_ms(50); // Delay 50ms
	LCD_WR_CMD(0x0012, 0x001C); // Internal reference voltage= Vci;
	delay_ms(50); // Delay 50ms
	LCD_WR_CMD(0x0013, 0x1A00); // Set VDV[4:0] for VCOM amplitude
	LCD_WR_CMD(0x0029, 0x0025); // Set VCM[5:0] for VCOMH
	LCD_WR_CMD(0x002B, 0x0000); // Set Frame Rate = 30
	delay_ms(50);// Delay 50ms
	// ----------- Adjust the Gamma Curve ----------//
	LCD_WR_CMD(0x0030, 0x0000);
	LCD_WR_CMD(0x0031, 0x0506);
	LCD_WR_CMD(0x0032, 0x0104);
	LCD_WR_CMD(0x0035, 0x0207);
	LCD_WR_CMD(0x0036, 0x000F);
	LCD_WR_CMD(0x0037, 0x0306);
	LCD_WR_CMD(0x0038, 0x0102);
	LCD_WR_CMD(0x0039, 0x0707);
	LCD_WR_CMD(0x003C, 0x0702);
	LCD_WR_CMD(0x003D, 0x1604);
	//------------------ Set GRAM area ---------------//
	LCD_WR_CMD(0x0060, 0xA700); // Gate Scan Line
	LCD_WR_CMD(0x0061, 0x0001); // NDL,VLE, REV
	//-------------- Partial Display Control ---------//
//	LCD_WR_CMD(0x006A, 0x0000); // set scrolling line
//	LCD_WR_CMD(0x0080, 0);
//	LCD_WR_CMD(0x0081, 0);
//	LCD_WR_CMD(0x0082, 100);
//	LCD_WR_CMD(0x0083, 100);
//	LCD_WR_CMD(0x0084, 0);
//	LCD_WR_CMD(0x0085, 100);
	//-------------- Panel Control -------------------//
	LCD_WR_CMD(0x0090, 0x0010);
	LCD_WR_CMD(0x0092, 0x0600);
//	LCD_WR_CMD(0x0007, 0x3033); // Screen ON, partial only

}

void LCD_Cmd_EnterSleep(void)
{
	LCD_WR_CMD(0x0007, 0x0131); // Set D1=0, D0=1
	delay_ms(10);
	LCD_WR_CMD(0x0007, 0x0130); // Set D1=0, D0=0
	delay_ms(10);
	LCD_WR_CMD(0x0007, 0x0000); // display OFF
	//************* Power OFF sequence **************//
	LCD_WR_CMD(0x0010, 0x0080); // SAP, BT[3:0], APE, AP, DSTB, SLP
	LCD_WR_CMD(0x0011, 0x0000); // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_WR_CMD(0x0012, 0x0000); // VREG1OUT voltage
	LCD_WR_CMD(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude
	delay_ms(200); // Dis-charge capacitor power voltage
	LCD_WR_CMD(0x0010, 0x0082); // SAP, BT[3:0], APE, AP, DSTB, SLP
}

void LCD_Cmd_ExitSleep(void)
{
	//*************Power On sequence ******************//
	LCD_WR_CMD(0x0010, 0x0080);
	LCD_WR_CMD(0x0011, 0x0000);
	LCD_WR_CMD(0x0012, 0x0000);
	LCD_WR_CMD(0x0013, 0x0000);
	LCD_WR_CMD(0x0007, 0x0001);
	delay_ms(200);
	LCD_WR_CMD(0x0010, 0x1490);
	LCD_WR_CMD(0x0011, 0x0227);
	delay_ms(50);
	LCD_WR_CMD(0x0012, 0x001C);
	delay_ms(50);
	LCD_WR_CMD(0x0013, 0x1A00);
	LCD_WR_CMD(0x0029, 0x0025);
	delay_ms(50);
	LCD_WR_CMD(0x0007, 0x0133);
}
