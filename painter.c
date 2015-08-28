/*
*
*  Under GPL License
*
*/

#include <stdlib.h>

#include "painter.h"



DrawLineContext _d_dl_ctx = {
	.bm = 0L
};

static GraphicContext _d_g_ctx = {
	.direction = 0,
	.invert = 0,
	.a = 1<<16,
	.b = 0,
	.c = 0,
	.d = 1<<16,
	.e = 0,
	.f = 0
};

/**
 * @brief  Init default context bitmask
 * @param  w:       width
 * @param  h:       height
 * @param  pattern: fillchar with pattern, 0 for none
 *                  try 0xaa, you may like it.
 */
bitmask Painter_SetupContextBitmask(u16 w, u16 h, u8 pattern){
	u16 i, size = (u32)w*(u32)h/8;
	if (_d_dl_ctx.bm != 0L) free(_d_dl_ctx.bm);
	_d_dl_ctx.bm = (bitmask)malloc(size);
	_d_dl_ctx.bmw = w;
	for (i=0;i<size;i++) _d_dl_ctx.bm[i] = pattern;
	_d_dl_ctx.bmx = _d_dl_ctx.bmy = 0;
	return _d_dl_ctx.bm;
}

// Move default context bitmask start point
void Painter_LocateContextBitmask(u16 x, u16 y){
	_d_dl_ctx.bmx =x;
	_d_dl_ctx.bmy =y;
}

// Relative move default context bitmask start point
void Painter_TranslateContextBitmask(s16 x, s16 y){
	_d_dl_ctx.bmx +=x;
	_d_dl_ctx.bmy +=y;
}

// W3C Canvas SetTransform, not implemented
void Painter_SetTransform(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f){
	_d_g_ctx.a = a;_d_g_ctx.b = b;_d_g_ctx.c = c;
	_d_g_ctx.d = d;_d_g_ctx.e = e;_d_g_ctx.f = f;
}

/**
 * @brief  Draw a line from start to end with linewith
 * @param  sx:   x of start point
 * @param  sy:   y of start point
 * @param  ex:   x of end point
 * @param  ey:   y of end point
 * @param  fc:   foreground color in RGB4444
 * @param  lw:   line width |<-lw->|
 * @param  flag: draw line options, available: PAINTER_DRAW_BM_HOLD
 * @return default context bitmask after line drawn
 */
bitmask Painter_DrawLine(u16 sx, u16 sy, u16 ex, u16 ey, u16 fc, u16 lw, u8 flag){

	#define ctx _d_dl_ctx

	lw = (lw==0)?1:lw;
	u16 lhw = (lw)/2+1;
	ctx.sx = sx;
	ctx.sy = sy;
	ctx.rx = (s16)ex - (s16)sx;
	ctx.ry = (s16)ey - (s16)sy;
	ctx.ll = ctx.rx*ctx.rx + ctx.ry*ctx.ry;
	//assert_param(ctx.ll>lw*lw);
	ctx.xhw = ctx.ry?(abs(ctx.rx)+abs(ctx.ry))*lhw/ctx.ry:abs(ctx.rx);
	ctx.yhw = ctx.rx?(abs(ctx.ry)+abs(ctx.rx))*lhw/ctx.rx:abs(ctx.ry);
	ctx.hard = ((lw-1) * (lw-1)+2)/4;
	ctx.soft = ((lw+1) * (lw+1)+2)/4;
	ctx.grad = ctx.soft - ctx.hard;
	ctx.lx0 = min(ctx.rx,0);
	ctx.lx1 = max(ctx.rx,0);
	ctx.ly0 = min(ctx.ry,0);
	ctx.ly1 = max(ctx.ry,0);
	ctx.fc = C_RGB4444toABAh5(fc);
	ctx.alpha32 = C_A15to32(C_ALPHA4(fc));

	if (0 == (flag & PAINTER_DRAW_BM_HOLD)) {
		Painter_SetupContextBitmask(abs(ctx.rx)+lhw+lhw+1, abs(ctx.ry)+lhw+lhw+1, 0);
		ctx.bmx += lhw + sx - min(sx, ex);
		ctx.bmy += lhw + sy - min(sy, ey);
	}

	LCD_SetWindow(min(sx,ex), min(sy,ey), abs(ctx.rx)+1, abs(ctx.ry)+1);
	if (ctx.ry == 0) LCD_DrawLineBody_Vertical(ctx);
	else if (ctx.rx == 0) LCD_DrawLineBody_Horizontal(ctx);
	else LCD_DrawLineBody(ctx);

	LCD_DrawLineEnd(ctx, sx, sy, ex, ey, lhw);
	ctx.bmx += ctx.rx;
	ctx.bmy += ctx.ry;
	ctx.sx = ex;
	ctx.sy = ey;
	ctx.rx = -ctx.rx;
	ctx.ry = -ctx.ry;
	LCD_DrawLineEnd(ctx, ex, ey, sx, sy, lhw);
	ctx.bmx += ctx.rx;
	ctx.bmy += ctx.ry;

	return ctx.bm;

	#undef ctx
}

/**
 * @brief  Draw a polygon describe in xs, ys
 * @param  xs:   sequence of x of points
 * @param  ys:   sequence of y of points
 * @param  size: sequence length
 * @param  fc:   foreground color
 * @param  lw:   line width |<-lw->|
 * @param  flag: draw line options, available: PAINTER_DRAW_BM_HOLD, PAINTER_DRAW_POLY_CLOSE
 * @return default context bitmask after polygon drawn
 */
bitmask Painter_DrawPoly(u16* xs, u16* ys, u16 size, u16 fc, u16 lw, u8 flag){
	u16 i, xMin, xMax, yMin, yMax;
	s16 omx, omy;

	#define ctx _d_dl_ctx

	lw = (lw==0)?1:lw;
	u16 lhw = (lw)/2+1;

	xMin = xMax = xs[0];
	yMin = yMax = ys[0];
	for (i=1;i<size;i++){
		update_min(xMin, xs[i]);
		update_min(yMin, ys[i]);
		update_max(xMax, xs[i]);
		update_max(yMax, ys[i]);
	}
	xMin -= lhw;yMin -= lhw;
	xMax += lhw;yMax += lhw;

	if (0 == (flag & PAINTER_DRAW_BM_HOLD)) {
		Painter_SetupContextBitmask(xMax-xMin+1, yMax-yMin+1, 0);
		ctx.bmx = xs[0] - xMin;
		ctx.bmy = ys[0] - yMin;
	}
	omx = ctx.bmx-xs[0]; omy = ctx.bmy-ys[0];

	flag |= PAINTER_DRAW_BM_HOLD;
	for (i=1;i<size;i++){
		Painter_LocateContextBitmask(omx+xs[i-1], omy+ys[i-1]);
		Painter_DrawLine(xs[i-1], ys[i-1], xs[i], ys[i], fc, lw, flag);
	}

	if (flag & PAINTER_DRAW_POLY_CLOSE){
		Painter_LocateContextBitmask(omx+xs[i-1], omy+ys[i-1]);
		Painter_DrawLine(xs[i-1], ys[i-1], xs[0], ys[0], fc, lw, flag);
	}

	ctx.bmx = omx+xs[0];ctx.bmy = omy+ys[0];

	return ctx.bm;
	#undef ctx
}

/**
 * @brief  Draw a circle with linewith lw
 * @param  cx:  x of center point
 * @param  cy:  y of center point
 * @param  r:   radius of circle
 * @param  fc:  foreground color
 * @param  lw:  line width |<-lw->|
 * @param  flag: draw line options, available: PAINTER_DRAW_BM_HOLD
 * @return default context bitmask after circle drawn
 */
bitmask Painter_DrawCircle(u16 cx, u16 cy, u16 r, u16 fc, u16 lw, u8 flag){
	u16 lhw = (lw)/2+1;

	if (0 == (flag & PAINTER_DRAW_BM_HOLD)) {
		Painter_SetupContextBitmask((r+lhw)*2+1, (r+lhw)*2+1, 0);
		_d_dl_ctx.bmx = r+lhw;
		_d_dl_ctx.bmy = r+lhw;
	}

	return LCD_DrawCircle(cx, cy, r, fc, lw, _d_dl_ctx.bm, _d_dl_ctx.bmx, _d_dl_ctx.bmy, _d_dl_ctx.bmw);
}

/**
 * @brief  Put image on screen with 8-bit mixing alpha a8
 * @param  data: image data
 * @param  left: image left pos
 * @param  top:  image top pos
 * @param  a8:   8-bit alpha
 */
void Painter_PutImage(const u16* data, u16 left, u16 top, u8 a8){
	u16 skip = data[0];
	u16 channel = data[1];
	u16 height = data[2];
	u16 width = data[3];

	LCD_SetWindow(left, top, width, height);
	switch (channel){
	case 3:
		if (a8 == 0xff) LCD_PutImage_RGB565(data + skip, width * height);
		else LCD_MixImage_RGB565(data + skip, width * height, a8);
		break;
	case 4:
		if (a8 == 0xff) LCD_PutImage_RGB4444(data + skip, width * height);
		else LCD_MixImage_RGB4444(data + skip, width * height, a8);
		break;
	default:
		break;
	}
}

/**
 * @brief  Mask image on screen with 8-bit mask alpha
 * @param  data: image data
 * @param  left: image left pos
 * @param  top:  image top pos
 * @param  a8:   8-bit alpha array as mask
 */
void Painter_MaskImage(const u16* data, u16 left, u16 top, const u8* a8){
	u16 skip = data[0];
	u16 channel = data[1];
	u16 height = data[2];
	u16 width = data[3];

	LCD_SetWindow(left, top, width, height);
	switch (channel){
	case 3:
		LCD_MaskImage_RGB565(data + skip, width * height, a8);
		break;
	case 4:
		LCD_MaskImage_RGB4444(data + skip, width * height, a8);
		break;
	default:
		break;
	}
}


/**
 * @brief  Clip and put image on screen according to bitmask
 * @param  data: image data
 * @param  left: image left pos
 * @param  top:  image top pos
 * @param  mask: bitmask
 */
void Painter_BitMaskImage(const u16* data, u16 left, u16 top, const bitmask mask){
	u16 skip = data[0];
	u16 channel = data[1];
	u16 height = data[2];
	u16 width = data[3];

	LCD_SetWindow(left, top, width, height);
	switch (channel){
	case 3:
		LCD_BitMaskImage_RGB565(data + skip, width * height, mask);
		break;
	case 4:
		LCD_BitMaskImage_RGB4444(data + skip, width * height, mask);
		break;
	default:
		break;
	}
}


/**
 * @brief  Put a single character on screen
 * @param  glyph: glyph data
 * @param  left:  char left pos
 * @param  top:   char top pos
 * @param  fc:    foreground color
 */
void Painter_PutChar(const u8* glyph, u16 left, u16 top, u16 fc){
	//head = skip, font_size, width;
	LCD_SetWindow(left, top, glyph[2], glyph[1]);
	LCD_PutChar_RGB4444(glyph + glyph[0], glyph[2]*glyph[1]/2, fc);
}


/**
 * @brief  Put a string on screen region, glyph and size info will be auto matched in resource.
 * @param  str:       the string
 * @param  font_size: font size
 * @param  fc:        foreground color
 * @param  bc:        background color, 0 to be transparent
 * @param  left:      string start left pos
 * @param  top:       string start top pos
 * @param  w:         text region width
 * @param  h:         text region height
 * @param  flag:      text render options, available: PAINTER_STR_SHADOW
 */
void Painter_PutString(const u16* str, u8 font_size, u16 fc, u16 bc,
					 u16 left, u16 top, u16 w, u16 h, u8 flag){
	u32 *fs_index_segment;
	u8 *glyph = 0L;

	u16 x, y, i;
	u16 sc = (((~fc)&0xfff0)|(fc&0xf>>1));

	i = 1;
	while (i && (font_size != res_glyph_index[i+1])) i = res_glyph_index[i];
	if (i == 0) return;
//	assert_msg(i>0, "No font of this size.");
	fs_index_segment = (u32*)res_glyph_index + i + 2;

	if (C_ALPHA4(bc)){
		if (!(flag & PAINTER_STR_SFLUSH)) LCD_FillRectangle_RGB4444(left, top, w, h, bc);
	}
	else {
			flag &= ~PAINTER_STR_SFLUSH;
	}

	x = y = i =0;
	while ((str[i])&&(y+font_size<=h)){
		x = 0;
		while ((str[i])&&(x<w)){
			glyph = (u8*)res_glyphs + fs_index_segment[str[i]-1];
			if (x+glyph[2]>w) {
				if (flag & PAINTER_STR_SFLUSH) LCD_FillRectangle_RGB4444(left+x, top+y, w-x, glyph[1], bc);
				break;
			}
			if (flag & PAINTER_STR_SFLUSH) LCD_FillRectangle_RGB4444(left+x, top+y, glyph[2], glyph[1], bc);
			if (flag & PAINTER_STR_SHADOW_M) {
					Painter_PutChar(glyph, left+x+(flag & PAINTER_STR_SHADOW_M),
								   top+y+(flag & PAINTER_STR_SHADOW_M), sc);
			}
			Painter_PutChar(glyph, left+x, top+y, fc);
			x += glyph[2];
			i++;
		}
		y += font_size;
	}
	if ((!str[i])&&(flag & PAINTER_STR_SFLUSH)) {
		LCD_FillRectangle_RGB4444(left+x, top+y-font_size, w-x, glyph?glyph[1]:h, bc);
		LCD_FillRectangle_RGB4444(left, top+y, w, h-y, bc);
	}
}

// BELOWS ARE BEING TESTED.

void Painter_Fill_Floodfill(u16 left, u16 top, u16 right, u16 bottom
							 , u16 sx, u16 sy, s16 mx, s16 my
							 , bitmask mask, u16 bmw, u16 fc, u8 flag){
	if (mask == 0L) {
		mask = _d_dl_ctx.bm;
		bmw = _d_dl_ctx.bmw;
	}

//	_UNUSED(flag);
	#define MAXLEN ((PAINTER_SCR_HEI + PAINTER_SCR_WID)*2)
	u16 qlen = MAXLEN;
	s16 qx[MAXLEN], qy[MAXLEN];

	if (flag & PAINTER_FILL_CONNECT8)
		LCD_Fill_Floodfill8_Core(left, top, right, bottom, sx, sy, mx, my
								 , mask, bmw, fc, qlen, qx, qy);
	else LCD_Fill_Floodfill4_Core(left, top, right, bottom, sx, sy, mx, my
							 , mask, bmw, fc, qlen, qx, qy);

//	free(qx);free(qy);

}


void Painter_Fill_BitMaskShadow(u16 left, u16 top, u16 right, u16 bottom
								, bitmask mask, s16 mx, s16 my, u16 bmw
								, u16 sc, s16 sx, s16 sy, s16 step5){
	if (mask == 0L) {
		mask = _d_dl_ctx.bm;
		bmw = _d_dl_ctx.bmw;
	}

	LCD_Fill_BitMaskShadow(left, top, right, bottom, mask, mx, my
						   , bmw, sc, sx, sy, step5);

}
