import sgmllib

class grabParser(sgmllib.SGMLParser):
    "Get all links and images from a given html string"

    def parse(self, s):
        "Parse the given string 's'."
        self.feed(s)
        self.close()

    def __init__(self, verbose=0):
        "Initialise an object, passing 'verbose' to the superclass."

        sgmllib.SGMLParser.__init__(self, verbose)
        self.hyperlinks = []
        self.imgs = []

    def start_a(self, attributes):
        "Process a hyperlink and its 'attributes'."

        for name, value in attributes:
            if name == "href":
                self.hyperlinks.append(value)

    def start_img(self, attributes):
        "Process a img and its 'attributes'."

        for name, value in attributes:
            if name == "src":
                self.imgs.append(value)
