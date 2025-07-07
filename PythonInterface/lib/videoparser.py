import os
import lib.parserinterface as pi

class VideoParser:
    """
    Class for interfacing with parser Interface on a high level
    """
    def __init__(self, input_file, dll_path):
        self.input_file = input_file
        self.frame_callback = None

        if not os.path.isfile(self.input_file):
            raise IOError("Input not found: " + str(input_file))

        self.parser = pi.ParserInterface(self.input_file, dll_path)
        self.collected_stats = []

    def get_stats(self):
        return self.collected_stats

    def set_frame_callback(self, callback):
        """
        Set the function to be called when a frame was successfully parsed
        """
        if callable(callback):
            self.frame_callback = callback
        else:
            raise RuntimeError("Callback not a callable function")

    def parse(self):
        """
        Parse the input file
        """
        self.parser.init()

        while True:
            # parse each frame
            try:
                stats = self.parser.parse_next_frame()
            except Exception as e:
                raise e

            # ignore empty stats
            if stats is None:
                continue

            # stop if result is "False", meaning end was reached
            if not stats:
                break

            self.collected_stats.append(stats)

            # pass bitstream data back to main program if needed
            if self.frame_callback is not None:
                self.frame_callback(stats)
