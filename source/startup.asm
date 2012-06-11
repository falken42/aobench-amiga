
        opt     l-,CHKIMM               ; auto link, optimise on

        section mycode,code             ; need not be in chipram

		xref	_main
		xdef	_SysBase

_SysBase        dc.l  0

_InternalMain:
        move.l  4.w,a6                  ; get SysBase
        move.l	a6,_SysBase
        jsr		_main
        rts                             ; back to workbench/cli
