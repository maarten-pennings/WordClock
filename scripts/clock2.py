# clock2.py - Creates clock2.gif, and animated GIF of a clock (minutes only) showing two rounding modes
# Maarten Pennings 2020 feb 29

import math
from PIL import Image, ImageDraw, ImageFont

# Angle factor (degree to radian)
Rad = 2*math.pi/360

# Size of the generated image
img_width= 800
img_height= 500

# Clock size
clk_radius= 200
clk_hand1_short= 30*clk_radius/100 # hand1 is the "real clock" (red in drawing)
clk_hand1_long= 75*clk_radius/100
clk_dial_radius= 80*clk_radius/100
clk_tick_size= 8*clk_radius/100
clk_hand2_short= 85*clk_radius/100 # hand2 is the clock quatized to 5 min intervals (green in drawing)
clk_hand2_long= 99*clk_radius/100

# Font, might need adaptation on you system
fnt = ImageFont.truetype('C:\Windows\Fonts\consola.ttf', 40)

# Draws a clock on `draw`, at position `cx`,`cy`. The time is `minute` (hours ignored). `mode` is either "round" or "floor"
def draw_clock(draw,cx,cy,minute, mode):    
    if mode=="floor" :
        delta1= 0.0 # arc length, in minutes, before tick
        delta2= 4.6 # arc length, in minutes, after tick
        delta3= 0.0 # offset, in minutes, fro quantized hand
    elif mode=="round":
        delta1=-2.3  # arc length, in minutes, before tick
        delta2=+2.3  # arc length, in minutes, after tick
        delta3= 2.5  # offset, in minutes, fro quantized hand
    else:
        print("Mode must be 'floor' or 'round'")

    # Draw clock circle segments
    for min in range(0,60,5):
        alpha1=(min+delta1)*6 # segment (arc) start (degree)
        alpha2=(min+delta2)*6 # segment (arc) end (degree)
        draw.arc( (cx-clk_dial_radius, cy-clk_dial_radius, cx+clk_dial_radius, cy+clk_dial_radius), alpha1, alpha2, fill='black', width=5) 
        
    # draw clock ticks
    for min in range(0,60,5):
        alpha= (min*6)*Rad # tick angle (radian)
        p1=( cx+(clk_dial_radius-clk_tick_size/2.0)*math.cos(alpha), cy+(clk_dial_radius-clk_tick_size/2.0)*math.sin(alpha) )
        p2=( cx+(clk_dial_radius+clk_tick_size/2.0)*math.cos(alpha), cy+(clk_dial_radius+clk_tick_size/2.0)*math.sin(alpha) )
        draw.line( [p1,p2], fill="black", width=5 ) 
    
    # draw the real hand
    alpha= (minute*6-90)*Rad # hand angle (radian) # note: 0deg is horizontal, so -90
    p1=( cx+clk_hand1_short*math.cos(alpha), cy+clk_hand1_short*math.sin(alpha) )
    p2=( cx+clk_hand1_long *math.cos(alpha), cy+clk_hand1_long *math.sin(alpha) )
    draw.line( [p1,p2], fill="red", width=5 )

    # draw the real time
    str= "XX:%02d" % minute
    (w,h)=draw.textsize(str, font=fnt)
    h=(h+2)//4*4 # stabilize height
    draw.text((cx-w/2,cy-h/2), str, font=fnt, fill="red")
    
    # draw the quantized hand
    min5= 5*((minute+delta3)//5) # quantize time
    alpha= (min5*6-90)*Rad # quantized hand angle (radian)
    p1=( cx+clk_hand2_short*math.cos(alpha), cy+clk_hand2_short*math.sin(alpha) )
    p2=( cx+clk_hand2_long *math.cos(alpha), cy+clk_hand2_long *math.sin(alpha) )
    draw.line( [p1,p2], fill="green", width=5 )
    
    # draw the quantized time
    str= "XX:%02d" % min5
    (w,h)=draw.textsize(str, font=fnt)
    h=(h+2)//4*4 # stabilize height
    draw.text((cx-w/2,cy-clk_radius-h*1.2), str, font=fnt, fill="green")
    
    # draw the quantized time
    str= "(" + mode + ")"
    (w,h)=draw.textsize(str, font=fnt)
    draw.text((cx-w/2,cy+clk_radius), str, font=fnt, fill="green")
    
# Create a list of frames for the animated GIF
list= []
for minute in range(0,60):
    # Create a white frame
    img= Image.new('RGB', (img_width,img_height), (255,255,255) )
    draw= ImageDraw.Draw(img)
    # Draw the two clocks
    draw_clock( draw, img_width/4, img_height/2, minute, "floor" )
    draw_clock( draw, 3*img_width/4, img_height/2, minute, "round" )
    # Append new frame to frame list
    list.append(img)

# Save frame list
list[0].save('clocks2.gif', save_all=True, append_images=list[1:], optimize=True, duration=500, loop=0)

