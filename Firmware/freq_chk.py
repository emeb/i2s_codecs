def freq():
    err  = 100
    for n in range(125, 267):
        for d1 in range(1, 8):
            for d2 in range(1, 8):
                f = (((6*n)/d1)/d2)
                if f<=200:
                    for cd in range(1,200):
                        mf = f/cd
                        if (mf >= 12.2) and (mf <= 12.3):
                            fs = (mf*1e6)/256
                            if(abs(fs-48000)< err):
                                err = abs(fs-48000)
                            print(f, mf, ":", n, d1, d2, cd, "--", fs, err)

freq()
