#import csv
import numpy as np

INF = 1e9
EPSILON = 1e-7
TWO_PI = 2 * np.pi

def convert_args(*args):
    if len(args) == 1:
        return args[0]
    elif len(args) == 2:
        return args[0] + args[1] * 1j
    else:
        raise ValueError("Arguments are not valid - specify either complex number z or real and imaginary part x, y")

def moebius_z(z, norm):
    return 1 - 2 * norm / (z + norm)

def moebius_inv_z(z, norm):
    return norm * (1 + z) / (1 - z)

def ang_to_c(ang):
    return np.cos(ang) + np.sin(ang) * 1j

def lambda_to_rad(lmb):
    return lmb * 4 * np.pi

def rad_to_lambda(rad):
    return rad * 0.25 / np.pi

def split_complex(z):
    return [np.real(z), np.imag(z)]

def vswr_rotation(x, y, impedance=1, real=None, imag=None, lambda_rotation=None, solution2=True, direction="clockwise"):
    '''Rotates the given point p=(x,y) to the given destination with
        the given orientation. Usually 2 solutions exist. If no
        solution exists, an ValueError exception is thrown.

        Keyword Arguments:

        *real*:
            rotates until real part of p is equal
            Accepts: non-negative float

        *imag*:
            rotates until imaginary part of p is equal
            Accepts: float

        *lambda_rotation*:
            rotates p the given amount (0.25 = 180deg)
            Accepts: float

        *solution2*:
            If real is set, uses solution with negative imaginary part.
            If imag is set, uses solution closer to infinity. Has no
            effect if none or lambda_rotation are set.
            Accepts: boolean

        *direction*:
            specifies rotation direction from p to destination
            Accepts: 'clockwise' or 'cw' and 'counterclockwise' or 'ccw'

        If no destination is set, a full turn from p to p is returned

        Returns: tuple (z0, z1, lambda)

        *z0*:
            input converted to complex number - z0 = x + y * 1j

        *z1*:
            endpoint as complex number

        *lambda*:
            rotation angle as part of lambda (0.5 = 180deg = Pi)
    '''
    if direction in ["clockwise", "cw"]:
        cw = True
    elif direction in ["counterclockwise", "ccw"]:
        cw = False
    else:
        raise ValueError("side has to be 'clockwise' or 'counterclockwise', resp. 'cw' or 'ccw'")

    #TODO: Does these default cases for invert, ang, and ang_0 make sense?
    invert = False
    ang_0 = 0.0
    ang = 0.0
    check = 0
    z = x + y * 1j
    z0 = moebius_z(z, impedance)

    if real is not None or imag is not None:
        a = np.abs(z0)

        if real is not None:
            assert real > 0
            check += 1

            b = 0.5 * (1 - moebius_z(real, impedance))
            c = 1 - b
            ang_0 = 0

            if real < 0 or abs(moebius_z(real, impedance)) > a:
                raise ValueError("given real destination is not reachable")

            invert = solution2

        if imag is not None:
            check += 1

            b = impedance / imag if imag != 0 else INF
            c = np.sqrt(1 + b ** 2)
            ang_0 = np.arctan(b)

            if c > a + abs(b):
                raise ValueError("given imag destination is not reachable")

            invert = solution2 != (imag < 0)


        gamma = np.arccos((a ** 2 + c ** 2 - b ** 2) / (2 * a * c)) % TWO_PI
        if invert:
            gamma = -gamma
        gamma = (ang_0 + gamma) % TWO_PI

        ang_z = np.angle(z0) % TWO_PI
        ang = (gamma - ang_z) % TWO_PI

        if cw:
            ang = ang - 2 * np.pi

    if lambda_rotation is not None:
        check += 1
        ang = lambda_to_rad(lambda_rotation)
        if cw:
            ang = -ang

    if check > 1:
        raise ValueError("method call not clear - too many destinations set")

    if check == 0:
        ang = TWO_PI

    return (z, moebius_inv_z(z0 * ang_to_c(ang), impedance), rad_to_lambda(ang))
