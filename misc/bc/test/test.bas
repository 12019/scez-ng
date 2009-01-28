Option Explicit

Declare ApplicationID = &HD2,&H76,&H00,&H00,&H92,&H11,&H11,&H0A,&00,&H00,_
	&H01,&H01,&H00
	' RID | 0x11 | 0x11 | SC_CARD_BASICCARD[2] | Ser. Num.[2] |
	' major version | minor version

#IfDef EnhancedBasicCard
Declare ATR = &H42,&H61,&H73,&H69,&H63,&H43,&H61,&H72,_
              &H64,&H20,&H5A,&H43,&H32,&H2E,&H33
#Else
Declare ATR = &H42,&H61,&H73,&H69,&H63,&H43,&H61,&H72,_
              &H64,&H20,&H5A,&H43,&H31,&H2E,&H31
#EndIf

#IfDef EnhancedBasicCard
Declare Key 0 = &H01,&H23,&H45,&H67,&H89,&HAB,&HCD,&HEF,_
                &H89,&HAB,&HCD,&HEF,&H01,&H23,&H45,&H67
Declare Key 1 = &H01,&H23,&H45,&H67,&H89,&HAB,&HCD,&HEF

Dir "\"
	File "File1"
End Dir
#Else
Declare Polynomials = &H5703CF4C, &HC7D5621F
Declare Key 0 = &H01,&H23,&H45,&H67,&H89,&HAB,&HCD,&HEF
#EndIf


