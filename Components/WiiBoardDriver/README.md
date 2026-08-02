# Why is this not a C++ file?
 - Because the refrence I'm using for this was found written in python only: [https://github.com/jmahmood/bbev/tree/main](https://github.com/jmahmood/bbev/tree/main)

# How does it work?
 - It just reads the raw data using the `bbev` python library which can read raw kernal data on linux, and then just looks for the device with the expected Wii Board name 
 - It then runs a local server on localhost:50000 which then [WiiBoardManager](../WiiBoardManager) can then just read through using the builtin FPrime TCP driver

# Was this driver adapted from that project by just using Gemini to do it for me and do it good for me 😩?
 - Yes...yes it was 😩😩😩🥵🥵