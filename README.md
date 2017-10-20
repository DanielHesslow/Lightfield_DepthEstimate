# Depth estimation for lightfields

For more details see blogpost here https://danielhesslow.github.io/light-fields/2017/10/14/playing_with_lightfields.html

The code is written such that you do not need to be very familiar with c++ or c.
That is, for educational purposes I've not used any part of the standard library, advanced features or even pointers.
Just the bare minimum, most of which could be written in java or C# or whatever.

I don't handle lightfields of arbritary size either, for the same reason, it introduces some complexity for not much educational gain.



# Compliation

I should really make this simpler, but at the moment I don't have the time.
So there's a few things missing here, dear imgui, handmade maths, and stb image. As well as glfw, glew, etc. 

Also lightfield images are missing.

Since i'm not sure if I can redistribute the images the process of making the compliations easier is currenlty not of much use. But nevertheless I should try to come up with something.

I also havn't tested it on other platforms so I'm sure it does not yet work, however changes _should_ be minimal.
Nor have I tested it on other GPU's so there's probably something else not working there. I had to work around one bug in my driver so I'm sure there's more out there to work around. 

So at the moment I'd concider it not worth your time. Feel free to look at it though. The fun parts are in the shaders.

Or just read the blog post.












