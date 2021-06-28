
def factorial(n):
    for i in range(n-1,0,-1):
        n *= i
    return n

if __name__=="__main__":
    for i in range(1,101):
        print("fac(%d) =" %i,factorial(i))