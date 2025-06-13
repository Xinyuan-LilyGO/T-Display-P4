import wave

def bytesarray2str(temp, sampwidth):
    """Convert a byte array to a string representation of the value based on sample width."""
    if sampwidth == 1:  # 8-bit audio
        value = temp[0]
        return f"0x{value:02X}"
    elif sampwidth == 2:  # 16-bit audio
        value = (temp[1] << 8) | temp[0]
        return f"0x{value:04X}"
    elif sampwidth == 3:  # 24-bit audio
        value = (temp[2] << 16) | (temp[1] << 8) | temp[0]
        return f"0x{value:06X}"
    elif sampwidth == 4:  # 32-bit audio
        value = (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0]
        return f"0x{value:08X}"
    else:
        raise ValueError(f"Unsupported sample width: {sampwidth}")

def get_wav_file(path):
    """Convert a WAV file to a C header file containing the audio data."""
    print(f"Processing WAV file: {path}")

    # Open the WAV file
    with wave.open(path, "r") as wave_read:
        # Get the parameters of the WAV file
        nchannels, sampwidth, samplerate, nframes, comptype, compname = wave_read.getparams()

        print(f"Channels: {nchannels}")
        print(f"Bits per sample: {sampwidth * 8}")
        print(f"Sample rate: {samplerate}")
        print(f"Number of frames: {nframes}")
        print(f"Compression type: {comptype}")
        print(f"Compression name: {compname}")

        # Generate the C header file name based on the WAV file parameters
        h_name = f"c{nchannels}_b{sampwidth * 8}_s{samplerate}"
        h_path = h_name + '.h'
        print(f"Output C header file: {h_path}")

        # Determine the appropriate C data type based on sample width
        if sampwidth == 1:
            c_type = "const uint8_t"
        elif sampwidth == 2:
            c_type = "const uint16_t"
        elif sampwidth == 3 or sampwidth == 4:
            c_type = "const uint32_t"
        else:
            raise ValueError(f"Unsupported sample width: {sampwidth}")

        # Open the output C header file for writing
        with open(h_path, 'w') as outFile:
            outFile.write("#include <stdint.h>\n")
            outFile.write(f"{c_type} {h_name}[] = {{\n")

            # Read and process each frame
            for i in range(nframes):
                # Read one frame (which contains nchannels samples)
                frame = wave_read.readframes(1)

                # Process each sample in the frame
                for j in range(nchannels):
                    # Extract the sample from the frame based on sample width
                    sample = frame[j * sampwidth:(j + 1) * sampwidth]
                    sample_str = bytesarray2str(sample, sampwidth)
                    outFile.write(sample_str)
                    outFile.write(',')

                # Add a newline after every 8 samples
                if (i * nchannels + j) % 8 == 7:
                    outFile.write('\n')

            # Add a terminating zero to the array
            outFile.write('0')
            outFile.write('};')

    print(f"Conversion complete. Output file: {h_path}")

if __name__ == "__main__":
    # Example usage
    wav_path = "input.wav"
    get_wav_file(wav_path)
