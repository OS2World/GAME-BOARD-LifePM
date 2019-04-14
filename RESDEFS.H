/*************************************************************************\
 * resdefs.h
 * Description: This file contains the definitions for the PM
 * resource routines (inclusivly help ids).
 *
 * (c) 1994 by Ralf Seidel
 * Set tabs to 3 to get a readable source.
\*************************************************************************/

/**********************************************************************\
 * General defintions
\**********************************************************************/


#define IDR_MAIN           100

/**********************************************************************\
 * Menu definitions.
\**********************************************************************/
#define IDM_FILE				500 
#define IDM_LOAD				501
#define IDM_SAVE				502
#define IDM_SAVEALL			503
#define IDM_EXIT				509
#define IDM_LIFE				510 
#define IDM_STEP				511
#define IDM_RUN				513
#define IDM_STOP				514
#define IDM_CLEAR				515
#define IDM_RANDOMIZE		516
#define IDM_OPTIONS			520
#define IDM_OPTALG			521
#define IDM_OPTALGREC		522
#define IDM_OPTALGSEQ		523
#define IDM_OPTTOP			524
#define IDM_OPTTOPFINIT		525
#define IDM_OPTTOPTORUS		526
#define IDM_OPTGRID			527
/* Don't change the order of the IDM_OPTSIZE* definitions */
#define IDM_OPTSIZE			530
#define IDM_OPTSIZE1x1		531
#define IDM_OPTSIZE2x2		532
#define IDM_OPTSIZE3x3		533
#define IDM_OPTSIZE4x4		534
#define IDM_OPTSIZE5x5		535
#define IDM_OPTSIZE6x6		536
#define IDM_OPTSIZE7x7		537
#define IDM_OPTSIZE8x8		538
#define IDM_OPTSIZE9x9		539
#define IDM_OPTSIZE10x10	540
#define IDM_EDIT				550
#define IDM_COPY				551
#define IDM_CUT				552
#define IDM_PASTE				553
#define IDM_HELP				590
#define IDM_HELP4HELP		594
#define IDM_ABOUT				595

/**********************************************************************\
 * Dialog definitions
\**********************************************************************/
#define IDD_OPTIONS			1000
#define IDC_CLASSICALG		1001
#define IDC_RECURSIVEALG	1002
#define IDC_FINITWORLD		1003
#define IDC_TORUSWORLD		1004

#define IDD_ABOUT				1100

/**********************************************************************\
 * String definitions
\**********************************************************************/
#define IDSTR_LOADDLGTITLE 					1500
#define IDSTR_SAVEDLGTITLE 					1501
#define IDSTR_HELPWINDOWTITLE					1502

/**********************************************************************\
 * Message string definitions
\**********************************************************************/
#define IDMSG_WININITIALIZE 					2000
#define IDMSG_WINCREATEMSGQUEUE  			2001
#define IDMSG_WINREGISTERCLASS 				2002
#define IDMSG_WINCREATESTDWINDOW 			2003
#define IDMSG_MALLOC 							2005
#define IDMSG_WINCREATEHELPINSTANCE			2010
#define IDMSG_WINASSOCIATEHELPINSTANCE		2011
#define IDMSG_CREATEMEMPS						2012
#define IDMSG_BEGINTHREAD 						2013
#define IDMSG_UNKOWNTHREADSTART				2014
#define IDMSG_UNKOWNTHREADEND					2015
#define IDMSG_WINDLGBOX							2016
#define IDMSG_MAKEBITMAP						2017
#define IDMSG_WINOPENCLIPBRD					2018
#define IDMSG_WINSETCLIPBRDDATA				2019
#define IDMSG_WINFILEDLG 						2020
#define IDMSG_GPIQUERYBITMAPINFOHEADER 	2022
#define IDMSG_BITMAPTOLARGE					2023
#define IDMSG_WRONGBITMAPFORMAT 				2024
#define IDMSG_WRONGSWITCHVALUE				2025

/**********************************************************************\
 * Definitions for the help system
\**********************************************************************/
#define HELP_TABLE			5000
#define STAB_MAIN				5001
#define STAB_OPTIONS			5002
#define STAB_LOADSAVE		5003
#define STAB_ABOUT			5004


#define PANEL_MAIN			5100
#define PANEL_FILE			5110
#define PANEL_LIFE			5120
#define PANEL_EDIT			5130
#define PANEL_OPTALG			5240
#define PANEL_OPTTOP			5241
#define PANEL_OPTSIZE		5242

#define PANEL_HELP			5150

#define PANEL_ABOUT			5200
#define PANEL_LOADSAVE		5210
#define PANEL_GNU				6000
#define PANEL_SELECTING		6010
#define PANEL_AUTHOR			6020
#define PANEL_BUGS			6050
#define PANEL_HISTORY		6051
#define PANEL_FUTURE			6052
#define PANEL_CREDITS		6053

