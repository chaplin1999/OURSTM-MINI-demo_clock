
#include "stm32f10x.h"
#include "stm32f10x_it.h"

#include "lcd_driver.h"
#include "res.h"
#include "painter.h"

void Delay(__IO uint32_t nCount);

#define DUMMY Delay(11345678)
#define LDUMMY {DUMMY;DUMMY;DUMMY;DUMMY;}

extern DrawLineContext _d_dl_ctx;

void long_shadow(){ // WE LOVE IT! NO ANTIALIAS!!
	s16 x, y;
	LCD_SetWindow(30, 30, 210, 210);
	for (y=30;y<240;y++)for (x=30;x<240;x++){
		if (((x-120)*(x-120)+(y-120)*(y-120)<119*119)&&
			(x>y-138)&&(x<y+138)&&(x+y>240))
				LCD_MixPixel_x32(0x0, 12);
		else
			LCD_GetPixel();
	}

}

int main(){

	// NEED OF KEY AND LED
	RCC_ClockSecuritySystemCmd(ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Init LCD
	LCD_Cmd_InitFSMC();
	LCD_Cmd_Init();
	LCD_Cmd_InitBacklight();

	// Put some background
//	LCD_FillRectangle_RGB565(0, 0, 240, 320, C_RGB565(0, 0, 0));
	Painter_PutImage(res_img_res_lp_jpg,0,0,0xff);

	// A circle to overlay long shadow
	LCD_FillCircle_RGB4444(120, 120, 100+18, 0x88fa);

	// now you can see it
	long_shadow();
	// an alternative shadow
//	LCD_FillCircle(120+1, 120+1, 100+2, 0x8886, 0);
//	LCD_FillCircle(120, 120, 100+2, 0x8886, 0);
//	LCD_FillCircle(120, 120, 100+1, 0x8886, 0);

	// Put my clock panel on screen
	LCD_FillCircle_RGB565(120, 120, 100, C_RGB565(0, 0, 0));
	LCD_FillCircle_RGB565(120, 120, 90, C_RGB565(0xff, 0xff, 0xff));

	// Clock frame need some shadow!
	Painter_SetupContextBitmask(210, 210, 0);
	u16 x, y, i;
	u32 t;
	for (x=0;x<200;x++) for (y=0;y<200;y++){
		t = (x-100)*(x-100)+(y-100)*(y-100);
		if ((t>=91*91)&&(t<100*100)) LCD_SetBitMask(_d_dl_ctx.bm, x, y, 210);
	}

	// Draw ticks and numbers
	const s8 TICK_XI[] = { 0,  42,  73,  85,  73,  42,   0, -42, -73, -85, -73, -42};
	const s8 TICK_YI[] = {-85, -73, -42,   0,  42,  73,  85,  73,  42,   0, -42, -73};
	const s8 TICK_XO[] = { 0,  44,  77,  90,  77,  45,   0, -44, -77, -90, -77, -45};
	const s8 TICK_YO[] = {-90, -77, -45,   0,  44,  77,  90,  77,  45,   0, -44, -77};
	const u16 *text[] = {res_string_0/*_FSTR_0*/,
			res_string_1/*_FSTR_1*/,
			res_string_2/*_FSTR_2*/,
			res_string_3/*_FSTR_3*/,
			res_string_4/*_FSTR_4*/,
			res_string_5/*_FSTR_5*/,
			res_string_6/*_FSTR_6*/,
			res_string_7/*_FSTR_7*/,
			res_string_8/*_FSTR_8*/,
			res_string_9/*_FSTR_9*/,
			res_string_10/*_FSTR_10*/,
			res_string_11/*_FSTR_11*/,
			res_string_12/*_FSTR_12*/
			};
	const u16 *col = res_string_13/*_FSTR_:*/;
	const u16 *space = res_string_14/*_FSTR_ */;
	u16 time[20];

	for (i=0;i<12;i++){
		Painter_LocateContextBitmask(100+TICK_XI[i], 100+TICK_YI[i]);
		Painter_DrawLine(120+TICK_XI[i], 120+TICK_YI[i], 120+TICK_XO[i], 120+TICK_YO[i], 0x000f, 5, PAINTER_DRAW_BM_HOLD);
		Painter_PutString(text[i?i:12], 14, 0x000f, 0
						 , 120+TICK_XI[i]-(TICK_XO[i]-TICK_XI[i])*2-4, 120+TICK_YI[i]-(TICK_YO[i]-TICK_YI[i])*2-6
						 , 20, 20, PAINTER_STR_SHADOW*2);
	}

	// Bravo!
	Painter_Fill_BitMaskShadow(30, 30, 230, 230, 0L, 10, 10, 210, 0x0008, 4, 4, 15);

	// LET IT RUN!
	u8 m, s; // so-called second and minute
	bitmask bm;
	_UNUSED(bm);
	s = 0; m = 0;
	Painter_SetupContextBitmask(200, 200, 0); // new context for new shadow
	Painter_LocateContextBitmask(100, 100); // start from center
	u8 shadow_on;
	u16 bg[200*60];
	// grab screen region, saving for later repaint
	LCD_SetWindow(20, 240, 200, 60);
	LCD_GetImage_RGB565(bg, 200*60);
	while (1){
		LCD_FillCircle_RGB565(120, 120, 65, 0xffff); // Clear clock hands, sucks aha~
		// Dont forget bitmasks
		for (x=30;x<170;x++) for (y=30;y<170;y++){
			LCD_ResetBitMask(_d_dl_ctx.bm, x, y, 200);
		}
		// Center circle makes it cute~
		Painter_DrawCircle(120, 120, 3, 0xa22f, 3, PAINTER_DRAW_BM_HOLD);
		// minute hand
		u8 mm = m % 12;
		Painter_DrawLine(120, 120
						 , 120+TICK_XI[mm]-(TICK_XO[mm]-TICK_XI[mm])*10
						 , 120+TICK_YI[mm]-(TICK_YO[mm]-TICK_YI[mm])*10,
						 0xa22f, 5, PAINTER_DRAW_BM_HOLD);
		Painter_DrawLine(120, 120
						 , 120-(TICK_XI[mm]+4)/8
						 , 120-(TICK_YI[mm]+4)/8,
						 0xa22f, 5, PAINTER_DRAW_BM_HOLD);
		Painter_DrawCircle(120, 120, 6, 0x6a2f, 4, PAINTER_DRAW_BM_HOLD);
		// second hand
		u16 sh_x[] = {120, 120+(TICK_XI[_mod(s+11,12)]+3)/5,
					  120+TICK_XI[s]-(TICK_XO[s]-TICK_XI[s])*6,
					  120+(TICK_XI[_mod(s+1,12)]+3)/5
					 };
		u16 sh_y[] = {120, 120+(TICK_YI[_mod(s+11,12)]+3)/5,
					  120+TICK_YI[s]-(TICK_YO[s]-TICK_YI[s])*6,
					  120+(TICK_YI[_mod(s+1,12)]+3)/5
					 };
		Painter_DrawPoly(sh_x, sh_y, 4, 0x6a2f, 3, PAINTER_DRAW_BM_HOLD | PAINTER_DRAW_POLY_CLOSE);
//		Painter_DrawLine(120, 120
//					, 120+TICK_XI[s]-(TICK_XO[s]-TICK_XI[s])*6
//					, 120+TICK_YI[s]-(TICK_YO[s]-TICK_YI[s])*6,
//					0x6a2f, 5, PAINTER_DRAW_BM_HOLD);
//		Painter_DrawLine(120, 120
//					 , 120-(TICK_XI[s]+4)/8
//					 , 120-(TICK_YI[s]+4)/8,
//				0x6a2f, 5, PAINTER_DRAW_BM_HOLD);

		// PUSH BUTTON TO RUN REALLY FAST, WITHOUT SHADOW!
		shadow_on = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15)?PAINTER_STR_SHADOW*2:0;
		// GET MY SHADOW!
		if (shadow_on) Painter_Fill_BitMaskShadow(50, 50, 190, 190, 0L, 30, 30, 200, 0x0008, 4, 4, 15);

		// show time in text
		u16 *p = time, *q;
		u16 t;
		// need some space
		*p++ = space[0];
		*p++ = space[0];
		*p++ = space[0];
		*p++ = space[0];
		*p++ = space[0];
		*p++ = col[0];
		//put second text directly
		q = (u16 *)(text)[s];
		while (*q) *p++ = *q++;
		*p++ = 0;

		p = time + 4;
		t = m;
		//put minute digits
		for(i=0;i<3;i++) {
			*(p--) = text[t%10][0];
			t /= 10;
		}
		// repaint the background
		LCD_SetWindow(20, 240, 200, 60);
		LCD_PutImage_RGB565(bg, 200*60);
		// GET MY TIME!
		Painter_PutString((const u16*)time, 48, 0xffff, 0xf5b9
						 , 20, 240, 200, 60, shadow_on);
		// PUSH A TO DASH!
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15)) DUMMY;
		// TICK TOCK~
		s++;
		if (s == 12) {
			m++;
			s = 0;
		}

	}

	return 0;

}










/**
 * @brief  Inserts a delay time.
 * @param  nCount: specifies the delay time length.
 * @retval None
 */
void Delay(__IO uint32_t nCount)
{
	for(; nCount != 0; nCount--);
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	// Flash Hall LEDs and send the message
	/* Infinite loop, flashing LEDs */
	while (1)
	{
	}
}

#endif
