# Why is this not a C++ file?
 - Because the refrence I'm using for this was found written in python only: [https://github.com/jmahmood/bbev/tree/main](https://github.com/jmahmood/bbev/tree/main)

# How does it work?
 - The old Python bridge read raw board events and forwarded them over localhost:50000.
 - That path is now replaced by [WiiBoardManager](../WiiBoardManager), which scans `/dev/input/event*`, finds the Wii Balance Board by name, and reads the `EV_ABS` events directly in C++.
 - The Python script is now only a reference for the original event-processing logic.

# Was this driver adapted from that project by just using Gemini to do it for me and do it good for me 😩?
 - Yes...yes it was 😩😩😩🥵🥵