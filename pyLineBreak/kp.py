# coding=utf-8
import json
import sys
from hyphenate import hyphenate_word
from afm import AFM

__author__ = 'alex'

TEX_INFINITY = 100000
HYPHEN_PENALTY = 50

UNICODE_TO_ADOBE = {
    8211: 177,  # en dash
    8212: 208,  # em dash
}


class Box(object):
    def __init__(self, content, w):
        self.content = content
        self.w = w

    def __repr__(self):
        return 'Box("%s", %s)' % (self.content, self.w)


class Glue(object):
    def __init__(self, w, y, z, content=''):
        self.content = content
        self.w = w
        self.y = y
        self.z = z

    def __repr__(self):
        return 'Glue(%s, %s, %s)' % (self.w, self.y, self.z)


class Penalty(object):
    def __init__(self, w=0.0, p=0, f=False, content=''):
        self.content = content
        self.w = w
        self.p = p
        self.f = f

    def __repr__(self):
        return 'Penalty("%s", %s, %s, %s)' % (self.content, self.w, self.p, self.f)


class FontMetric(object):
    def __init__(self, metric_file):
        self.metric = AFM(open(metric_file))

    @staticmethod
    def _map_to_adobe(char):
        c = ord(char)
        if c in UNICODE_TO_ADOBE:
            return UNICODE_TO_ADOBE[c]
        return c

    def get_width(self, char, size):
        c = self._map_to_adobe(char)
        return size * (self.metric.get_width_char(c, isord=True) / 1000.0)

    def get_kern_distance(self, c1, c2, size):
        c1 = self._map_to_adobe(c1)
        c2 = self._map_to_adobe(c2)
        return size * (self.metric.get_kern_dist(c1, c2, isord=True) / 1000.0)


class Paragraph(object):
    def __init__(self, items):
        self.items = items
        self.lines = []
        self._breakpoints = None

    def is_legal_breakpoint(self, pos):
        """
            From BPIL, a legal breakpoint is one of:
                - Penalty with p < infinity
                - Glue directly preceded by a box
        """
        this_item = self.items[pos]
        prev_item = self.items[pos - 1] if pos > 0 else None
        return (isinstance(this_item, Penalty) and this_item.p < TEX_INFINITY) or (
            isinstance(this_item, Glue) and isinstance(prev_item, Box))

    def get_breakpoints(self):
        if self._breakpoints is not None:
            return self._breakpoints

        self._breakpoints = [0] + filter(self.is_legal_breakpoint, range(len(self.items)))

        return self._breakpoints

    def get_line_metrics(self, start, end):

        line = self.items[start:end]

        length = 0
        stretch = 0
        shrink = 0

        for item in line:
            if isinstance(item, (Box, Glue)):
                length += item.w
            if isinstance(item, Glue):
                stretch += item.y
                shrink += item.z

        last_item = line[len(line) - 1]

        if isinstance(last_item, Glue):  # Get rid of trailing glue
            length -= last_item.w
        elif isinstance(last_item, Penalty):  # Add width of penalty
            length += last_item.w

        return {
            'length': length,
            'stretch': stretch,
            'shrink': shrink,
        }

    def first_fit(self, width, justify=False):
        legal_bps = self.get_breakpoints()
        start = 0
        end = 1
        lines = []
        while end < len(legal_bps) - 1:
            while end < len(legal_bps) and self.get_line_metrics(legal_bps[start], legal_bps[end])['length'] < width:
                end += 1
            if end >= len(legal_bps):
                end = len(legal_bps) - 1
            elif start + 1 < end and self.get_line_metrics(legal_bps[start], legal_bps[end])['length'] > width:
                end -= 1
            start_item = legal_bps[start] + 1
            end_item = legal_bps[end] + 1
            line = self.items[start_item:end_item]
            line_width = self.get_line_metrics(start_item, end_item)['length']
            if justify:
                extra_space = width - line_width
                glue_items = [item for item in line if isinstance(item, Glue)]
                glue_width = sum([item.w for item in glue_items])
                stretch_factor = 1 + (extra_space / glue_width)
                for glue_item in glue_items:
                    glue_item.w *= stretch_factor
            lines.append((line_width, line))
            start = end
            end += 1

        return lines


def html_encode(character):
    if ord(character) < 128:
        return character
    return '&#%d;' % ord(character)


def tokenise(text, size, metric_file, indent=1, explicit_kerning=True, collapse_boxes=True):
    """
        Treat whatever we're passed as one paragraph
    """

    items = []
    font_metric = FontMetric(metric_file)

    default_space_glue_params = {
        'w': font_metric.get_width(' ', size),
        'y': font_metric.get_width(' ', size) / 2.0,
        'z': font_metric.get_width(' ', size) / 3.0,
    }

    if indent:
        items.append(Box('', indent * size))

    for word in text.split():
        last_c = None
        for i, word_part in enumerate(hyphenate_word(word)):
            if i > 0:  # Append a flagged penalty for the hyphen
                items.append(Penalty(p=HYPHEN_PENALTY, w=font_metric.get_width('-', size), f=True, content='-'))
            for c in word_part:

                # Kern
                if explicit_kerning and last_c is not None and font_metric.get_kern_distance(last_c, c, size) != 0:
                    items.append(Box(w=font_metric.get_kern_distance(last_c, c, size), content=''))

                # Add box
                items.append(Box(html_encode(c), font_metric.get_width(c, size)))

                # Add penalty for explicit hyphens
                if c in (u'-', u'–', u'—'):
                    items.append(Penalty(p=HYPHEN_PENALTY, f=True))
                last_c = c
        items.append(Glue(**default_space_glue_params))

    if collapse_boxes:
        new_items = []
        curr_box = None
        for item in items:
            if isinstance(item, Box):
                if item.content == '':  # It's a spacing adjustment
                    if curr_box is not None:
                        new_items.append(curr_box)
                        curr_box = None
                    new_items.append(item)

                elif curr_box is None:
                    curr_box = item
                else:
                    curr_box.content += item.content
                    curr_box.w += item.w

            else:
                if curr_box is not None:
                    new_items.append(curr_box)
                    curr_box = None
                new_items.append(item)

        if curr_box is not None:
            new_items.append(curr_box)

        items = new_items

    items.append(Penalty())
    items.append(Glue(w=0, y=TEX_INFINITY, z=0))
    items.append(Penalty(p=-TEX_INFINITY))

    return items


def to_json(lines):
    layout_data = []
    for width, items in lines:
        this_line = []
        offset = 0
        for item in items:
            # Want to print item, unless it's a penalty that's not at the end of a line
            # sys.stderr.write("%s [%s, %s]\n" % (type(items[0]).__name__, type(items[-2]).__name__, type(items[-1]).__name__))
            if len(item.content) and (not isinstance(item, Penalty) or item is items[-1]):
                this_line.append({'position': "%.3f" % offset, 'width': "%.3f" % item.w, 'word': item.content})
            if not isinstance(item, Penalty):
                offset += item.w
        layout_data.append(this_line)
    return json.dumps(layout_data, indent=4)


if __name__ == '__main__':

    if len(sys.argv) < 5:
        sys.stderr.write("Not enough args you dumbass\n")
        sys.exit(1)

    in_file = sys.argv[1]
    font_file = sys.argv[2]
    font_size = float(sys.argv[3])
    col_wid = float(sys.argv[4]) * 72

    if in_file == '-':
        text = sys.stdin.readlines()
    else:
        with open(in_file) as f:
            text = f.readlines()

    text = ''.join(text)
    items = tokenise(text, font_size, font_file)
    par = Paragraph(items)
    lines = par.first_fit(col_wid, justify=True)

    json = to_json(lines)
    print json

