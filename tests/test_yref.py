from os.path import dirname, join

def data(p):
    return open(join(dirname(__file__),p),'r').read()


def test_yref():
    from yref import refer

    dd = refer([
      [False, "recursion-3", data("req-3.yaml"), 1, "vas/ymvas"],
      [False, "recursion-2", data("req-2.yaml"), 0, "vas/ymvas"],
      [True , "recursion-1", data("req-1.yaml"), 0, "vas/ymvas"]
    ])

    print(dd)
