# WordClock
A clock that tells time in plain text

## Introduction
I got the idea from [here](http://www.espruino.com/Tiny+Word+Clock).
It uses a simple 8x8 LED matrix, so very little mechanics to do.
The downside is that 8x8 LEDs means we are very restricted on the _model_: 
which words are placed where and how.

## Model
I want a Dutch word clock.
I do not like vertical text. Is it possible to fit all this on 8x8?

For the hours, we need words `one` to `twelve`. That is 48 characters - note we are writing `vyf`, not `vijf`.
This means that we can not have words for all 60 minutes. Let's try to go for multiples of 5 only.
In Dutch this means `vyf` (`over`), `tien` (over), `kwart` (over), tien (voor `half`), vyf (voor half), half, vyf (`over` half), etc.
That is 24 characters.

Oops, 24+48 is 72, and we only have 64. 
But we can save a bit, for example, for `tien` and `negen`, we only need `tienegen`.

### Model 1
For my first model, I made a graph, coupling first letters to last letters of words. I started with the minutes

![Minutes graph](imgs/minutes.png)

We can only save 1 character, but that doesn't help, we go from 24 to 23. But 24 is 3 rows, and saving one character 
on the last row doesn't help, we can not fit an hour there.

Two solutions are shown below. They fit in three rows.
```
  vyfkwart     kwartien
  tienvoor     vyf_voor
  overhalf     overhalf
```

Note that `vyf`, `tien`, and `kwart` need to come before `over` and `voor`, and only then we can have `half`.
So there is not much room for alternatives.

Let's next look at the hours. They need to come after the minutes.
This is the graph.

![Hours graph](imgs/hours.png)

We have a problem. Full words require 48 characters. We can best save 5, which still requires 43. 
But we only have 40 (5 rows), so we need to get rid of 3 more characters.

I cheated in my first model: (1) two paired letters vIEr and twaaLF, (2) one split word Z-E-S. 
A minor problem in the first model is that two times a space missing 
(between `tien` and `voor`, and between `over` and `half`) 
and the word `uur` missing (for every full hour).

This is my first attempt.

![model 1](imgs/model1.jpg)


### Model 2.
Marc relaxed the rules, he allows diagonal words. He wrote a solver algorithm and found the below solution.

![model 2](imgs/model2.jpg)

This eliminates the paired letters and split words. Still a missing space, and still `uur` missing.


## Prototype
First prototype was made with an ESP8266.
I made a [video](https://www.youtube.com/watch?v=YDhCZarNm9g) that runs 
at approximately 600x so that all states appear in a one minute movie.

I also made a [real clock](WordClock) and a
[video](https://youtu.be/wVqeRSxwd_Y) that captures one state change.
This really keeps the time (based on the ESP8266 crystal).
At startup the user can press the FLASH button to set the hour and minute.

The next version of uses the second words model (Marc's).
At startup you can not only set hour and minute, but also mode: clock or a fast demo.
Here is the [video](https://www.youtube.com/watch?v=LO9IB6KRluM) of the fast mode.




