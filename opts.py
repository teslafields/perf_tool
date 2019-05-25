import getopt, sys

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:o:v:", ["help", "output="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err) # will print something like "option -a not recognized"
        sys.exit(2)
    output = None
    verbose = False
    for o, a in opts:
        print(o, a)
        if o == "-v":
            verbose = True
        elif o in ("-o", "--output"):
            output = a
    # ...

if __name__ == "__main__":
    main()
