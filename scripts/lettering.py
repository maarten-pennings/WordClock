
class Words:
    #wordss = [ ["vyf","tien","kwart"], ["voor","over"], ["half"], ["een","twee","drie","vier","vijf","zes","zeven","acht","negen","tien","elf","twaalf"] ]
    wordss = [ ["vyf","tien","kwart"], ["voor","over"], ["half"], ["een","twee","drie","vier","vijf","zes","zeven","acht"] ]
    def reset(self):
        self.maskss=[]
        for words in self.wordss:
            self.maskss.append( list(map(lambda word:True,words)) )
    def __init__(self):
        self.reset()
        #print( self.wordss )
        #print( self.maskss )
    def print(self):
        for words,masks in zip(self.wordss,self.maskss):
            for word,mask in zip(words,masks):
                ch= "+" if mask else "-"
                print("%s%s" % (word, ch), end=" " )
            print()
    def level(self):
        for ix, masks in enumerate(self.maskss):
            if any(masks) : return ix
        return -1

#  +--+-----------------------+--+
#  |00|01 02 03 04 05 06 07 08|09|
#  +--+-----------------------+--+
#  |10|11 12 13 14 15 16 17 18|19|
#  |20|21 22 23 24 25 26 27 28|29|
#  |30|31 32 33 34 35 36 37 38|39|
#  |40|41 42 43 44 45 46 47 48|49|
#  |50|51 52 53 54 55 56 57 58|59|
#  |60|61 62 63 64 65 66 67 68|69|
#  |70|71 72 73 74 75 76 77 78|79|
#  |80|81 82 83 84 85 86 87 88|89|
#  +--+-----------------------+--+
#  |90|91 92 93 94 95 96 97 98|99|
#  +--+-----------------------+--+
  
class Board:
    wall= '#'
    blank= '-'
    stub= 'x'
    def __init__(self,width,height):
        self.width= width
        self.height= height

        self.first = self.width+3
        self.last = (self.width+2) * (self.height+1) -1
        
        self.horizontal = +1
        self.slopeup= self.width+1
        self.slopedn = self.width+3

        self.stubcount= 0
        self.cells = []
        for x in range(self.width+2): 
            self.cells.append(self.wall)
        for y in range(self.height):
            self.cells.append(self.wall)
            for x in range(self.width): self.cells.append(self.blank)
            self.cells.append(self.wall)
        for x in range(self.width+2): 
            self.cells.append(self.wall)
        self.counts= [0] * len(self.cells)
    def print(self,pos=-1,indent=""):
        for ix,cell in enumerate(self.cells):
            if ix % (self.width+2) == 0: print(indent,end="")
            if ix==pos:
                ch= cell.upper()
                if cell=='-': ch='+'
                if cell=='#': ch='*'
                print(ch,end="")
            else:
                print(cell,end="")
            if ix % (self.width+2) == self.width+1: print("")
    def fit(self,pos,word,dir):
        for ix,letter in enumerate(word):
            val=self.cells[pos+ix*dir]
            if val!=self.blank and val!=letter: return False
        return True
    def put(self,pos,word,dir):
        for ix,letter in enumerate(word):
            self.cells[pos+ix*dir]=letter
            self.counts[pos+ix*dir] += 1
    def rem(self,pos,word,dir):
        for ix,letter in enumerate(word):
            self.counts[pos+ix*dir] -= 1
            assert( self.counts[pos+ix*dir]>=0 )
            assert( self.cells[pos+ix*dir]==letter )
            if self.counts[pos+ix*dir]==0: self.cells[pos+ix*dir]= self.blank
    def fitstub(self,pos):
        return self.cells[pos]!=board.wall and self.stubcount<2
    def putstub(self,pos):
        if self.cells[pos]==board.blank: 
            board.cells[pos]=board.stub
            self.stubcount +=1
    def remstub(self,pos):
        if board.cells[pos]==board.stub: 
            board.cells[pos]=board.blank
            self.stubcount -=1
    
            
        
def solve(board,words,pos):
    level= words.level()
    if level<0: 
        print("SOLVED")
        board.print()
        return
    if pos>=board.last: return
    while board.cells[pos]==board.wall: pos+=1
    for ix, word in enumerate(words.wordss[level]):
        if words.maskss[level][ix]:
            if board.fit(pos,word,board.horizontal):
                words.maskss[level][ix]= False
                board.put(pos,word,board.horizontal)
                solve(board,words,pos+1)
                board.rem(pos,word,board.horizontal)
                words.maskss[level][ix]= True
            if board.fit(pos,word,board.slopedn):
                words.maskss[level][ix]= False
                board.put(pos,word,board.slopedn)
                solve(board,words,pos+1)
                board.rem(pos,word,board.slopedn)
                words.maskss[level][ix]= True
            if board.fitstub(pos) :
                board.putstub(pos)
                solve(board,words,pos+1)
                board.remstub(pos)
    

words= Words()
board= Board(8,8)

solve(board,words,board.first)

