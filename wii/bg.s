.globl piclength
.globl picdata

piclength: .long picdataend - picdata
picdata:
.incbin "../wii/bg.jpg"
picdataend:
