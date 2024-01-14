	.TITLE test
	.PSECT PMAIN
	; бит CLK (часы для плеера)
	CLKMSK=1
	; бит IN (входные данные для плеера)
	INMSK=2
	; бит OUT (выходные данные плеера)
	OUTMSK=1
	; Начальная задержка в 1 секунду
	STARTDL=23310.
.=.+100000
	jmp main
print:
	nop
	rts pc
	; --------
	; Задержка
	; r0 - сколько раз по 42.9 микросекунд ждать
	; портит: r0
delay:
	mov r0,@#177706
	mov #177712,r0
	mov #24,(r0)	; 24 - запуск таймера + режим установки бита 7 при переходе через ноль
0wait:
	tstb (r0)
	bpl 0wait		; если бит 7 не установлен, то байт положителный
	rts pc

	; --------
	; задержка в 1 секунду
	; портит: r0
del1s:	
	mov #STARTDL,r0
	call delay
	rts pc

	; --------
	; задержка в 2 секунды
	; портит: r0
del2s:
	mov #STARTDL*2,r0
	call delay
	rts pc

	; --------
	; Задержка при посылке одного бита
	; определяет скорость обмена
	; портит: -
xfcdly:	
    nop
	rts pc

	; --------
	; Посылка/передача одного бита
	; Флаг C бит для передачи и принятый бит
	; портит: -
bitrw:
	mov r1,-(sp)
	mov #177714,r1
	bcs 1clk
	mov #0,(r1)			; сначала бит для передачи = 0 и часы = 0
	mov #CLKMSK,(r1)	; теперь  бит для передачи = 0 и часы = 1 (теперь плеер может считывать бит)
	bit #OUTMSK,(r1)    ; считывает что передал плеер
	beq 1out0           ; оставляем C = 0 потому что плеер передал 0
	sec                 ; плеер передал 1
1out0:	
	call xfcdly	        ; держим часы 1
	mov #0,(r1)			; держим бит для передачи = 0, но часы = 0
	br 0exit
1clk:
	mov #INMSK,(r1)			; сначала бит для передачи = 1 и часы = 0
	mov #INMSK+CLKMSK,(r1)	; теперь  бит для передачи = 1 и часы = 1 (теперь плеер может считывать бит)
	bit #OUTMSK,(r1)		; считывает что передал плеер
	bne 1out1				; оставляем C = 1 потому что плеер передал 1
	clc						; плеер передал 0
1out1:	
	call xfcdly				; держим часы 1
	mov #INMSK,(r1)			; держим бит для передачи = 1, но часы = 0
0exit:
	call xfcdly				; держим часы 0
	mov (sp)+,r1
	rts pc

	; --------
	; Послать один байт
	; r0 - байт
	; портит: r0, r1
sendb:
	mov #8.,r1
0loop:
	rorb r0
	call bitrw
	sob r1, 0loop
	rts pc

	; --------
	; Принять один байт
	; r0 - байт
	; портит: r0, r1
recvb:
	clr r0
	mov #8.,r1
1loop:
	sec						; приём байт происходит подачей команды CMD_NOP (377) плееру
	call bitrw
	rorb r0
	sob r1, 1loop
	rts pc

	; --------
	; Дождаться ответа плеера, где он скажет сколько байт ещё нужно считать
	; Внимание: много команд возвращает длину 0, то есть дополнительных данных нет, 
	; это в частности означает, что не стоит делать SOB по длине без проверки на ноль.
	; r0 - нужно принять число байт
	; портит: -
getlen:
	call  recvb
	tstb r0
	bpl getlen
	bic #177600,r0
	rts pc

	; ===================================================================
	; Обработка команд.
	; Это не чать интерфейса, это примеры использования команд.
	; -------------------------------------------------------------------
	; Самый длинный ответ плеера не больше 40 символов. Для примера используем этот буфер.
buf=4000
	; *******************************************************************
	; CMD_NOP, код 377
	; ничего не делает, используется неявно внутри recvb - эта команда заставляет
	; плеер передать байт, который у него возможно есть в очереди.

	; *******************************************************************
	; CMD_RESET, код 1
	; ничего на самом деле не делает, но возвращает строку с версией прошивки.
	; Хороший кандидат для проверки функционирования связи с плеером.
	; Длина возвращаемой строки не больше 40 символов. 
creset:
	; послать команду CMD_RESET
	mov #1,r0
	call  sendb
	; прочитать длину ответа
	call getlen
	tst r0
	bne 1crst
	jmp err
1crst:	
	; будем писать строку ответа в buf
	mov #buf,r3
	mov r0,r2
0crst:	
	call recvb
	movb r0,(r3)+
	sob r2,0crst
	; заканчиваем строку нулевым байтом
	clrb (r3)
	; вывести строку из buf на экран
	mov #buf,r0
	mov #2,r5
	call print
	rts pc
	
	; *******************************************************************
	; CMD_GET_MODEL, код 2
	; возвращает строку с названием модели привода CDROM.
	; Длина возвращаемой строки не больше 40 символов. 
cmodel:
	; послать команду CMD_GET_MODEL
	mov #2,r0
	call  sendb
	; прочитать длину ответа
	call getlen
	tst r0
	bne 2cmdl
	jmp err
2cmdl:	
	; будем писать строку ответа в buf
	mov #buf,r3
	mov r0,r2
0cmdl:	
	call recvb
	movb r0,(r3)+
	sob r2,0cmdl
	; заканчиваем строку нулевым байтом
	clrb (r3)
	; вывести строку из buf на экран
	mov #buf,r0
	mov #2,r5
	call print
	rts pc

	; *******************************************************************
	; CMD_EJECT, код 3
	; открывает или закрывает лоток привода.
	; возвращает символ 
	;  O - привод открывает лоток
	;  L - привод закрывает лоток
	; Это позволяет выводить куда-нибудь строку статуса типа 
	; "OPENING..." (открываемся) или "LOADING..." (загружаем диск)
opstr:	.asciz 'Opening...'	
lostr:	.asciz 'Loading...'
	.even
cmejct:
	; послать команду CMD_EJECT
	mov #3,r0
	call  sendb
	; прочитать длину ответа
	call getlen
	tst r0
	bne 2eject
	jmp err
2eject:	
	; для одного символа буфер не нужен, будем принимать решение основываясь на r0
	call recvb
	cmpb #'O,r0
	bne 0eject
	mov #opstr,r0
	br 1eject
0eject:	
	mov #lostr,r0
1eject:
	; вывести строку из buf на экран
	mov #2,r5
	call print
	rts pc
	; *******************************************************************
	; CMD_GET_DISK, код 4
	; запрашивает тип диска в приводе.
	; возвращает символ 
	;  A - нормальный аудио CD
	;  U - неизвестный тип диска
	;  O - лоток открыт
	; Может использоваться для вывода надписи 'NO DISK', а также для проверки
	; открылся ли лоток после подачи команды CMD_EJECT
austr:	.asciz 'AUDIO CD'	
opestr:	.asciz 'Tray is open'
nodstr: .asciz 'NO DISK'
	.even
cmgdsk:
	; послать команду CMD_GET_DISK
	mov #4,r0
	call  sendb
	; прочитать длину ответа
	call getlen
	tst r0
	bne 5gdisk
	jmp err
5gdisk:	
	; для одного символа буфер не нужен, будем принимать решение основываясь на r0
	call recvb
	cmpb #'A,r0
	bne 0gdisk
	mov #austr,r0
	br 1gdisk
0gdisk:	
	cmpb #'U,r0
	beq 2gdisk
	; не печатаем об открытом лотке, но возвращаем -1 в r0
	mov #-1,r0
	rts pc
2gdisk:	
	mov #nodstr,r0
1gdisk:
	; вывести строку из buf на экран
	mov #2,r5
	call print
	clr r0
	rts pc
	; *******************************************************************
	; CMD_GET_TRACK_NUM, код 5
	; запрашивает количество дорожек диска.
	; возвращает на одну дорожку больше: последняя дорожка указывает на
	; последнюю минуту/секунду/кадр последней песни.
	.even
trstr:	.asciz 'tracks.'
cmgtn:
	; послать команду CMD_GET_TRACK_NUM
	mov #5,r0
	call sendb
	; прочитать длину ответа
	call getlen
	tst r0
	beq err
	; для одного числа буфер не нужен, будем принимать решение основываясь на r0
	call recvb
	mov r0,-(sp) ; запоминаем сколько дорожек чтобы потом знать сколько раз запрашивать информацию о них
	; вывести число из R0 на экран без перевода строки
	mov #3,r5
	call print
	; выводим 'tracks.'
	mov #trstr,r0
	mov #2,r5
	call print
	mov (sp)+,r0
	rts pc
	; *******************************************************************
	; CMD_GET_TRACK_INFO, код 6
	; запрашивает информацию о конкретной дорожке. 
	; Это команда с параметром - номером дорожки, параметр посылается вторым байтом
	; за кодом команды.
	; в R0 указываем номер дорожки
	; возвращает 4 байта в следующем порядке:
	; байт 0: Номер дорожки - это номер, присвоенный дорожке производителем диска, необязательно
	; упорядоченный и непрерывный, там может быть числа от 1 до 99. Специальная дорожка
	; 170 указывает на последние минуту/секунд/кадр последей песни диска.
	; байт 1: Начальная минута дорожки
	; байт 2: Начальная екунда дорожки
	; байт 3: Начальный кадр дорожки  (каждая секунда содержит 75 кадров)
emstr: .byte 0
	.even
cmgti:
	mov r2,-(sp)
	mov r0,-(sp)
	; послать команду CMD_GET_TRACK_INFO
	mov #6,r0
	call sendb
	mov (sp)+,r0
	; послать параметр - номер дорожки
	call sendb
	; прочитать длину ответа
	call getlen
	tst r0
	beq err
	; будем сразу выводить 4 байта ответа на экран
	mov r0,r2
0cmgti:	
	call recvb
	; вывести число из R0 на экран без перевода строки
	mov #3,r5
	call print
	sob r2,0cmgti
	; перевод строки
	mov #emstr,r0
	mov #2,r5
	call print
	mov (sp)+,r2
	rts pc
	; *******************************************************************
	; CMD_PLAY_TRACK, код 7
	; играет дорожки с начальной по конечную не включая.
	; Номера дорожек начинаются с 0 и соответствуют полученным данным по CMD_GET_TRACK_INFO.
	; Это команда с двумя параметрами - номером первой дорожки и номером дорожки, которую уже не играть.
	; Номера посылаются сразу после команды.
	; Именно здесь пригодится "несуществующая" последняя дорожка - если указать её номер в качестве
	; второго параметра, то будет проиграны дорожки с начальной до конца диска.
	; в R0 указываем номер ночальной дорожки, в R1 - номер дорожки, которую уже не играть.
cmplay:
	mov r1,-(sp)
	mov r0,-(sp)
	; послать команду CMD_PLAY_TRACK
	mov #7,r0
	call sendb
	mov (sp)+,r0
	; послать параметр - номер начальной дорожки
	call sendb
	; послать параметр - номер дорожки, до которой играть
	mov (sp)+,r0
	call sendb
	; прочитать длину ответа и игнорировать
	call getlen
	rts pc
	; *******************************************************************
	; CMD_STATUS, код 10
	; получить текущее состояние плеера. Самая важная команда, желательно
	; вызывать не реже 10 раз в секунду.
	; Команды NEXT и PREV полагаются на данные последнего вызова CMD_STATUS
    ; для решения какая дорожка играется сейчас.
	; Возвращает 5 байт в следующем порядке:
	; байт 0: состояние диска ()
	; байт 1: номер текущей дорожки (присвоенный производителем)
	; байт 2: текущая минута
	; байт 3: текущая секунда
	; байт 4: текущий кадр
	; для проверочных целей возвращаем в r0 состояние диска
cmstat:
	; послать команду CMD_STATUS
	mov #10,r0
	call sendb
	; прочитать длину ответа
	call getlen
	tst r0
	beq err
	; будем сразу выводить 5 байт ответа на экран
	mov r2,-(sp)
	mov r0,r2
	dec r2
	call recvb
	mov r0,-(sp)
0cmsta:	
	; вывести число из R0 на экран без перевода строки
	mov #3,r5
	call print
	call recvb
	sob r2,0cmsta
	; перевод строки
	mov #emstr,r0
	mov #2,r5
	call print
	mov (sp)+,r0
	mov (sp)+,r2
	rts pc
	; *******************************************************************
	; CMD_PAUSE, код 11
	; приостанавливаем воспроизведение. Байтов ответа нет поэтому игнорируем нулевую длину ответа.
cmpaus:
	; послать команду CMD_PAUSE
	mov #11,r0
	call sendb
	; прочитать длину ответа и игнорировать
	call getlen
	rts pc
	; *******************************************************************
	; CMD_RESUME, код 12
	; продолжаем воспроизведение. Байтов ответа нет поэтому игнорируем нулевую длину ответа.
cmresm:
	; послать команду CMD_RESUME
	mov #12,r0
	call sendb
	; прочитать длину ответа и игнорировать
	call getlen
	rts pc
	; *******************************************************************
	; CMD_NEXT, код 14
	; Переходим на следующую дорожку. Байтов ответа нет поэтому игнорируем нулевую длину ответа.
	; Как таковой команды NEXT у привода нет, поэтому здесь плеер ориентируясь на каталог диска
	; даёт команду PLAY приводу.
	; Поэтому если нужно переходить на следующую дорожку когда воспроизведение остановлено то
	; после NEXT нужно сразу дать команду PAUSE.
cmnext:
	; послать команду CMD_NEXT
	mov #14,r0
	call sendb
	; прочитать длину ответа и игнорировать
	call getlen
	rts pc

	; ===================================================================
	; ошибка
err:
	; вывести сообщение
	; print("Ответ плеера содежит длину 0, которая недопустима для текущей команды.")
	halt
	; ===================================================================
main:
	; стек
	mov #1000,sp
	; Начальный сброс: часы 1, ждём ~ секунду, часы 1
	mov #1, @#177714
	; Обязательно. Небольшая задержка чтобы плеер успел приготовиться
	; даже если таймер сработает на второй раз это не имеет значения.
	call del1s
	mov #0, @#177714

	; ============
	; тестовая часть
	; чистим экран
	mov #40000,r0
	mov #20000,r1
2loop:
	clr (r0)+
	sob r1, 2loop

	; тест CMD_RESET
	call creset
	; запрашиваем и печатает модель привода
	call cmodel
	; открываем лоток
	call cmejct
3loop:	
	call del1s
	call cmgdsk ; проверяем какой диск в приводе и открыт ли лоток
	tst r0
	beq 3loop
	; закрываем лоток
	call cmejct
	
	call del1s
	; запрашиваем число дорожек
	call cmgtn 
	; в цикле запрашиваем и печатаем информацию о каждой дорожке
	mov r0,r2
	clr r3
4loop:
	mov r3,r0
	call cmgti
	inc r3
	sob r2,4loop

	; играем с третьей дорожки по пятую 
	mov #2,r0
	mov #4,r1
	call cmplay

	; запрашиваем состояние плеера, пригодится для NEXT/PLAY
	call cmstat

	; приостанавливаем вопроизведение через 6 секунд
	call del2s
	call del2s
	call del2s
	call cmpaus

	; продолжаем играть через 4 секунды
	call del2s
	call del2s
	call cmresm

	; через 4 секунды переходим к следующей дорожке
	call del2s
	call del2s
	call cmnext
	; нужно запросить статус поскольку любой NEXT/PREV основывается на последнем вызове CMD_STATUS
	call cmstat

	; через 4 секунды приостанавливаем воспроизведение и переходим к следующей дорожке
	call del2s
	call del2s
	call cmpaus
	call cmnext
	; через 2 секунды начинаем играть
	call del2s
	call cmresm



	br .
	.END

