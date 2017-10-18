
import argparse

def parse_args():
    parser = argparse.ArgumentParser(
        description="Yandex Data School Keyboard Swipe challenge solutions checker",
    )
    parser.add_argument("--target", dest="target", required=True)
    parser.add_argument("--submission", dest="submission", required=True)
    return parser.parse_args()

args = parse_args()

targetLines = filter(lambda x : len(x) > 1, open(args.target).readlines())
submissionLines = filter(lambda x : len(x) > 1, open(args.submission).readlines())

tasksCount = len(targetLines)
rightAnswersCount = 0

for i in range(tasksCount):
    if i >= len(submissionLines):
        break
    if targetLines[i].strip().lower() == submissionLines[i].strip().lower():
        rightAnswersCount += 1

print float(rightAnswersCount) / tasksCount
