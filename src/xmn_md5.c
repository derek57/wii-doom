//#include <pspdebug.h>
#include <stdio.h>
#include "md5.h"
#include "doomdef.h"

//#define printf pspDebugScreenPrintf

extern char		target[MAXPATH];

char calculated_md5_string[33];
char known_md5_string_chex_quest_iwad[33] = "25485721882b050afa96a56e5758dd52";
char known_md5_string_doom_beta_1_4_iwad[33] = "a21ae40c388cb6f2c3cc1b95589ee693";
char known_md5_string_doom_beta_1_5_iwad[33] = "e280233d533dcc28c1acd6ccdc7742d4";
char known_md5_string_doom_beta_1_6_iwad[33] = "c428ea394dc52835f2580d5bfd50d76f";
char known_md5_string_doom_share_1_0_iwad[33] = "90facab21eede7981be10790e3f82da2";
char known_md5_string_doom_share_1_1_iwad[33] = "52cbc8882f445573ce421fa5453513c1";
char known_md5_string_doom_share_1_2_iwad[33] = "30aa5beb9e5ebfbbe1e1765561c08f38";
char known_md5_string_doom_share_1_25s_iwad[33] = "17aebd6b5f2ed8ce07aa526a32af8d99";
char known_md5_string_doom_share_1_666_iwad[33] = "762fd6d4b960d4b759730f01387a50a1";
char known_md5_string_doom_share_1_8_iwad[33] = "5f4eb849b1af12887dec04a2a12e5e62";
char known_md5_string_doom_share_1_9_iwad[33] = "f0cefca49926d00903cf57551d901abe";
char known_md5_string_doom_reg_1_1_iwad[33] = "981b03e6d1dc033301aa3095acc437ce";
char known_md5_string_doom_reg_1_2_iwad[33] = "792fd1fea023d61210857089a7c1e351";
char known_md5_string_doom_reg_1_6_iwad[33] = "7b18a4e70a52bb95a70286929cc2b135";
char known_md5_string_doom_reg_1_6b_iwad[33] = "464e3723a7e7f97039ac9fd057096adb";
char known_md5_string_doom_reg_1_666_iwad[33] = "54978d12de87f162b9bcc011676cb3c0";
char known_md5_string_doom_reg_1_8_iwad[33] = "11e1cd216801ea2657723abc86ecb01f";
char known_md5_string_doom_reg_1_9_iwad[33] = "1cd63c5ddff1bf8ce844237f580e9cf3";
char known_md5_string_doom_reg_1_9ud_iwad[33] = "c4fe9fd920207691a9f493668e0a2083";
char known_md5_string_doom_bfg_psn_iwad[33] = "e4f120eab6fb410a5b6e11c947832357";
char known_md5_string_doom2_1_666_iwad[33] = "30e3c2d0350b67bfbf47271970b74b2f";
char known_md5_string_doom2_1_666g_iwad[33] = "d9153ced9fd5b898b36cc5844e35b520";
char known_md5_string_doom2_1_7_iwad[33] = "ea74a47a791fdef2e9f2ea8b8a9da13b";
char known_md5_string_doom2_1_7a_iwad[33] = "d7a07e5d3f4625074312bc299d7ed33f";
char known_md5_string_doom2_1_8_iwad[33] = "c236745bb01d89bbb866c8fed81b6f8c";
char known_md5_string_doom2_1_8f_iwad[33] = "3cb02349b3df649c86290907eed64e7b";
char known_md5_string_doom2_1_9_iwad[33] = "25e1459ca71d321525f84628f45ca8cd";
char known_md5_string_doom2_bfg_xbox360_iwad[33] = "f617591a6c5d07037eb716dc4863e26b";
char known_md5_string_final_doom_tnt_old_iwad[33] = "1d39e405bf6ee3df69a8d2646c8d5c49";
char known_md5_string_final_doom_tnt_new_iwad[33] = "4e158d9953c79ccf97bd0663244cc6b6";
char known_md5_string_final_doom_plutonia_old_iwad[33] = "3493be7e1e2588bc9c8b31eab2587a04";
char known_md5_string_final_doom_plutonia_new_iwad[33] = "75c8cf89566741fa9d22447604053bd7";
char known_md5_string_freedoom_0_6_4_iwad[33] = "5292a1275340798acf9cee07081718e8";
char known_md5_string_freedoom_0_7_rc_1_beta_1_iwad[33] = "e2db4f21fbcfd0d69e39ae16b0594168";
char known_md5_string_freedoom_0_7_iwad[33] = "21ea277fa5612267eb7985493b33150e";
char known_md5_string_freedoom_0_8_beta_1_iwad[33] = "0597b0937e9615a9667b98077332597d";
char known_md5_string_freedoom_0_8_iwad[33] = "e3668912fc37c479b2840516c887018b";
char known_md5_string_freedoom_0_8_phase_1_iwad[33] = "a4ba85037ef3f91604c2281d9bb98a57";
char known_md5_string_freedoom_0_8_phase_2_iwad[33] = "bbbe0bc6b219664c94f192158dc54f19";
char known_md5_string_hacx_share_1_0_iwad[33] = "d2918f9ba48b16e7128b89b61e4de359";
char known_md5_string_hacx_reg_1_0_iwad[33] = "1511a7032ebc834a3884cf390d7f186e";
char known_md5_string_hacx_reg_1_1_iwad[33] = "b7fd2f43f3382cf012dc6b097a3cb182";
char known_md5_string_hacx_reg_1_2_iwad[33] = "65ed74d522bdf6649c2831b13b9e02b4";

int MD5_Check(char *final)		// FOR PSP: THIS FUNCTION DEFINITELY WORKS, BUT IT WAS NEVER USED - MAYBE FUTURE
{
    int i;
    int bytes;
//    char *filename = target;
    unsigned char c[MD5_DIGEST_LENGTH];
    unsigned char data[1024];

    FILE *inFile = fopen (final, "rb");

    MD5_CTX mdContext;

//    if (inFile == NULL)
//    {
//        printf("%s can't be opened.\n", filename);

//        return 0;
//    }

    MD5_Init(&mdContext);

    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);

    MD5_Final(c, &mdContext);

    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
	sprintf(&calculated_md5_string[i * 2], "%02x", (unsigned int)c[i]);
/*
    if(strncmp(calculated_md5_string, known_md5_string_hexen_1_1_iwad, 32) == 0)
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\nMD5 MATCH!\n");
    else
	printf("\nMD5 FAIL!\n");

    printf("%s\n", known_md5_string_hexen_1_1_iwad);

    printf("%s\n", calculated_md5_string);

    printf("%s\n", final);
*/
    fclose(inFile);

    return 0;
}

