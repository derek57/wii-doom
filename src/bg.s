.globl piclength
.globl picdata

piclength: .long picdataend - picdata
picdata:
.incbin "../src/bg.jpg"
picdataend:
