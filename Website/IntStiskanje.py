def seznam_razlik(seznam): 
    razlika_seznam=[]
    razlika_seznam.append(seznam[0])
    for i in range(1, len(seznam)):
        razlika=seznam[i]-seznam[i-1]
        razlika_seznam.append(razlika)
    return razlika_seznam

def kodiranje_ponovitev(binary,enaki):
    c=binary+"01"
    biti= format(enaki, "03b")
    d=c+biti
    return d

def kodiranje_abs_vrednost(binary, st):
    c=binary+"10"
    if st<0:
        stevilo=abs(st)
        d=c+"1"
        biti= format(stevilo, "08b")
        return d+biti
    else:
        d=c+"0"
        biti= format(st, "08b")
        return d+biti
    
def kodiranje_razlika(binary, st):
    c=binary+"00"
    if st>-3 and st<3:
        l=[-2,-1,1,2]
        n=0
        while(n<len(l)):
            if st==l[n]:
                break
            else:
                n=n+1
                
        d=c+"00"
        biti= format(n, "02b")
        return d+biti
    elif st>-7 and st<7:
        l=[-6,-5,-4,-3,3,4,5,6]
        n=0
        while(n<len(l)):
            if st==l[n]:
                break
            else:
                n=n+1
                
        d=c+"01"
        biti= format(n, "03b")
        return d+biti
    elif st>-15 and st<15:
        l=[-14,-13,-12,-11,-10,-9,-8,-7,7,8,9,10,11,12,13,14]
        n=0
        while(n<len(l)):
            if st==l[n]:
                break
            else:
                n=n+1
                
        d=c+"10"
        biti= format(n, "04b")
        return d+biti
    elif st>-31 and st<31:
        l=[-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,-16,-15,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30]
        n=0
        while(n<len(l)):
            if st==l[n]:
                break
            else:
                n=n+1
                
        d=c+"11"
        biti= format(n, "05b")
        return d+biti
        
def kodiranje(lista):
    seznam=seznam_razlik(lista)
    prvo=seznam[0]
    binary = format(prvo, "08b") 
    velikost=1
    while(velikost<len(seznam)):
        i=1
        enaki=0
        while(i<9):
            if(velikost+i==len(seznam)):
                break
            if(seznam[velikost]==0):
                if (seznam[velikost]==seznam[velikost+i]):
                    i=i+1
                    enaki=enaki+1
                else:
                    break
            else:
                break
        if(enaki==0):
            if(seznam[velikost]==0):
                binary=kodiranje_ponovitev(binary,enaki)
            elif(seznam[velikost]<-30) or (seznam[velikost]>30):
                stevilo=seznam[velikost]
                binary=kodiranje_abs_vrednost(binary, stevilo)
            elif(seznam[velikost]>-31) or (seznam[velikost]<31):
                stevilo=seznam[velikost]
                binary=kodiranje_razlika(binary, stevilo)
                
        elif(enaki>0):
            binary=kodiranje_ponovitev(binary,enaki)
        velikost=velikost+i
    koncno=binary+ "11"

    
    i = 0
    n = 0
    podatki = [len(koncno)%8]
    num = 0
    while i < len(koncno):
        if koncno[i] == '1':
            num |= 1<<n
        n += 1
        i += 1
        if n > 7:
            n = 0
            podatki.append(num)
            num = 0
    if num > 0:
        podatki.append(num)

    return podatki, koncno
