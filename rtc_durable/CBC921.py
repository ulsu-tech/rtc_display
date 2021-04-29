import pyb, micropython
import utime
from pyb import I2C

micropython.alloc_emergency_exception_buf(100)

class CBC921(object) :
    def __init__(self,port, speed):
        self.i2c = I2C(port)
        self.i2c.init(I2C.MASTER,baudrate = speed)
        #devs = self.i2c.scan()
        #if len(devs) == 1 && devs[0] == 0x55 :
        #self.dev = devs[0]
        self.dev = 0x55

    # send one byte to address
    def put(self,a,v) :
        #vs = bytearray(1)
        #vs[0]=v
        self.i2c.mem_write(v,self.dev,a)

    # receive one byte from address (integer)
    def get(self,a) :
        v = self.i2c.mem_read(1,self.dev,a)
        return v[0]

    # get all registers
    def damp(self) :
        m = self.i2c.mem_read(0x24+1,self.dev,0x00)
        i = 0
        for z in m :
            s1 = hex(i)
            s2 = hex(z)
            print(s1,' '*(4-len(s1)),':',s2,' '*(4-len(s2)),' | ', end='  ')
            i = i + 1
            if i % 8 == 0 :
                print()
        print()

    #get temperature
    def termo(self) :
        r22 = self.get(0x22)
        r22 = r22 | 0x80
        self.put(0x22,r22)
        utime.sleep(1)
        r23 = self.get(0x23)
        if r23 & 0x80 == 0x80 :
            t = self.i2c.mem_read(2,self.dev,0x1E)
            print(t[0]*2+t[1]/128-273)

    # on/off CLKOUT pin (12)
    def clkout(self,mode) :
        r13 = self.get(0x13)
        if mode == True :
            r13 |= 0x40
        else :
            r13 &= ~0x40
        self.put(0x13,r13)

    # get datetime
    def gett(self,alarm) :
        self.put(0x1C,0x60)
        self.put(0x22,0x01)     # LATCH _OUT = 1
        adr = 0x00
        if alarm == True :
            adr = 0x08
        b = self.i2c.mem_read(8,self.dev,adr)
        c = []
        j = 0
        for i in b :
            d,e = divmod(i,16)
            c.append(d*10+e)
            j += 1
        return(c)

    # set datetime cbc.putt([ss,sec,min,hour,day,month,year,day_of_week],alarm)
    # alarm = 0 set RTC registers, alarm = 1 set Alarm registers
    def putt(self,t,alarm) :
        adr = 0x00
        if alarm == True :
            adr = 0x08
        self.put(0x1C,0x60)
        b = bytearray(len(t))
        j = 0
        for i in t :
            d,e = divmod(i,10)
            b[j] = d*16+e
            j += 1
        w = self.get(0x10)
        w |= 0x01
        self.put(0x10,w)     # WRTC = 1
        self.i2c.mem_write(b,self.dev,adr)   # send to datetime registers
        self.put(0x22,0x02)     # LOAD_DAT_IN = 1 save datetime to shadow
        w &= ~0x01
        self.put(0x10,w)     # WRTC = 0

    # CDT count down timer set & run
    def cdt_set(self,v) :
        self.put(0x1A,v)        # CDTINIT value
        r18 = (0x07 << 2) | (0x01) | (0x40) # 1 Hz | 1 Hz | CDRPT repeat
        self.put(0x18,r18)

    # CDT count down timer get current value
    def cdt_get(self) :
        r22 = self.get(0x22)
        r22 |= 0x04             # LATCH_CDT_OUT
        self.put(0x22,r22)
        r19 = self.get(0x19)
        return(r19)

    def cdt(self,mode) :
        r18 = self.get(0x18)
        if mode == True :
            r18 |= 0xC0         # CDTE=1 enable CDRPT=1
        else :
            r18 &= ~0x80        # stop
        self.put(0x18,r18)
