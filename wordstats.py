from collections import defaultdict
import sys

__author__ = 'alex'


def get_word_stats():
    file_name = sys.argv[1]
    words = defaultdict(lambda: 0)
    word_count = 0

    with open(file_name) as f:
        content = f.read()
        for word in content.split():
            words[word] += 1
            word_count += 1

    cumulative = 0
    for count, word in sorted([(c, w) for w, c in words.items()], reverse=True):
        # print "%s %s" % (count, word)
        cumulative += count
        print float(cumulative) / float(word_count)

    print "Total words: %s" % word_count
    print "Total unique: %s" % len(words)


if __name__ == '__main__':
    get_word_stats()
