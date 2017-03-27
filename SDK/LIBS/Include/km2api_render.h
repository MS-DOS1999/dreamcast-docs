/*++

  COPYRIGHT        (C) NEC        Corporation 1997 - 1999

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author     :		T.Hirata(NEC Software,Ltd.)
  Module Name:		km2api_render.h

  Abstract:
	KAMUI = Kamui is abstruction model that is ultimate interface for PowerVR.
	'km2api_render.h' define the function of rendering and the function 
	which is related with renderring.

  Environment:
	ANSI C Compiler.

  Notes:
	kmRender and kmRenderTexture must be among kmEndScene from kmBeginScene();
	kmRender and kmRenderTexture return RenderID.
	Therefore,RenderID must be examined in kmGetRenderStatus if wanting to know
	result of kmRender and kmRenderTexture.

  Revision History:

--*/

#if !defined(__kamui2_render_api_h__)
#define __kamui2_render_api_h__

/*++
kmRender

proto type:
	KMINT32
	kmRender( 
			IN KMDWORD dwFlipMode 
		);

Description:
	�^�C���A�N�Z�����[�^�ɑ΂��āA1�V�[�����̒��_�f�[�^�̓o�^���I���������Ƃ�_�Ђɒʒm��,
	�f�[�^�W�J������ABack Buffer �ɑ΂��郌���_�����O���J�n���܂��B

Arguments:
	dwFlipMode

Result:
	KMINT32		0 < (return)	DMA lounch success. you can query finish render,
				0 > (return)	miss.

--*/

KMINT32 KMAPI
kmRender( 
		IN KMDWORD dwFlipMode
	);

/*++
kmRenderRect

proto type:
	KMINT32
	kmRenderRect( 
			IN KMDWORD dwFlipMode 
		);

Description:
	�^�C���A�N�Z�����[�^�ɑ΂��āA1�V�[�����̒��_�f�[�^�̓o�^���I���������Ƃ�_�Ђɒʒm��,
	�f�[�^�W�J������ABack Buffer �ɑ΂��郌���_�����O���J�n���܂��B�����_�����O�O�ɁA
	���[�W�����A���C�̍č\�z���s���܂��B

Arguments:
	dwFlipMode

Result:
	KMINT32		0 < (return)	DMA lounch success. you can query finish render,
				0 > (return)	miss.

--*/

KMINT32 KMAPI
kmRenderRect( 
		IN KMDWORD dwFlipMode
	);

/*++
kmRenderTexture

proto type:
	KMINT32
	kmRenderTexture(	
			IN	PKMSURFACEDESC pTextureSurface,
			IN  KMDWORD        dwDitherMode
		);

Description:
	�^�C���A�N�Z�����[�^�ɑ΂��āA1�V�[�����̒��_�f�[�^�̓o�^���I���������Ƃ�_�Ђɒʒm���܂��B
	���_�f�[�^�W�J������A����(pTextureSurface) �Ŏw�肳��ꂽ�e�N�X�`���ɑ΂��郌���_�����O���J�n���܂��B(
	�Ȃ��A�`���Ɏw�肷��e�N�X�`���́ARectangle/Stride�`���łȂ���΂Ȃ�܂���B

Arguments:
	OUT	pTextureSurface		�����_�����O���ʂ��i�[����e�N�X�`���ł��B

Result:
	KMINT32		0 < (return)	DMA lounch success. you can query finish render,
				0 > (return)	miss.

--*/

KMINT32 KMAPI
kmRenderTexture(
			IN	PKMSURFACEDESC pTextureSurface,
			IN  KMDWORD        dwDitherMode
		);

/*++
kmGetRenderStatus

proto type:
	KMSTATUS
	kmGetRenderStatus(	
			IN	KMINT32			nRenderID
		);

Description:
	���s���������_�����O�����݂ǂ̂悤�ȏ󋵂��𒲂ׂ܂��B

Arguments:
	IN nRenderID	kmRender��������kmRenderTexture���s�����ꍇ�̕Ԃ�l����͂��܂��B

		���jkmRenderTexture�����kmRender�ɂ�NO_FLIP���w�肵���ꍇ�A
		�@�@KMSTATUS_UNDER_DISPLAY�͖{�֐��̕Ԃ�l�Ƃ͂Ȃ�܂���B

Result:
	KMSTATUS_UNDER_DMA				�w���ID���������_�����O�͌��ݒ��_�f�[�^��DMA�]����
	KMSTATUS_FINISH_DMA				�w���ID���������_�����O�͌���DMA�]�����I��
	KMSTATUS_UNDER_RENDER			�w���ID���������_�����O�͌��݃����_�����O��
	KMSTATUS_FINISH_RENDER			�w���ID���������_�����O�͌��݃����_�����O���I��
	KMSTATUS_UNDER_DISPLAY			�w���ID���������_�����O�͌��ݕ\����
	KMSTATUS_FINISH_ALL_SEQUENCE	�w���ID���������_�����O�͌��݂��ׂĂ̏������I��

--*/

KMSTATUS KMAPI
kmGetRenderStatus(	
		IN	KMINT32			nRenderID
	);

#endif