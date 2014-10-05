import unittest
from kp import *


__author__ = 'alex'


class TestKP(unittest.TestCase):

    def setUp(self):
        self.items = [
            Box('a', 5),    # 0
            Glue(1, 1, 1),  # 1
            Glue(1, 1, 1),  # 2
            Box('a', 5),    # 3
            Penalty(),      # 4
            Penalty(p=TEX_INFINITY),  # 5
            Glue(1, 1, 1),  # 6
            Box('a', 5),    # 7
        ]

        self.p = Paragraph(self.items)

    def test_is_legal_breakpoint(self):
        tests = [
            (0, False),
            (1, True),
            (2, False),
            (3, False),
            (4, True),
            (5, False),
            (6, False),
            (7, False),
        ]

        for index, expected in tests:
            self.assertEquals(self.p.is_legal_breakpoint(index), expected, msg="Pos %s" % index)

    def test_get_breakpoints(self):
        self.assertEquals(self.p.get_breakpoints(), [0, 1, 4])

if __name__ == '__main__':
    unittest.main()