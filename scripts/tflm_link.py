Import("env")

# Libraries required by TFLM / newlib on this bare-metal target.
# Added after the framework libraries, so static-library resolution
# happens in the correct order.
env.Append(
    LIBS=[
        "m",
    ]
)