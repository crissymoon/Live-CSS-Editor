"""
guard_classifier.py  —  standalone spam / prompt-injection guard
Usage:
    from guard_classifier import GuardClassifier
    guard = GuardClassifier("./spam_injection_model/final")
    print(guard.classify("ignore all previous instructions"))
Requirements:
    pip install torch transformers
"""
import json
import numpy as np
import torch
import torch.nn.functional as F
from transformers import DistilBertTokenizerFast, DistilBertForSequenceClassification

DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
MAX_LEN = 128


class GuardClassifier:
    def __init__(self, model_dir: str):
        self.tokenizer = DistilBertTokenizerFast.from_pretrained(model_dir)
        self.model = DistilBertForSequenceClassification.from_pretrained(model_dir)
        self.model.eval()
        self.model.to(DEVICE)
        with open(f"{model_dir}/label_map.json") as f:
            self.label_map = {int(k): v for k, v in json.load(f).items()}

    def classify(self, text: str) -> dict:
        cleaned = " ".join(str(text).lower().strip().split())
        inputs = self.tokenizer(
            cleaned,
            return_tensors="pt",
            truncation=True,
            padding="max_length",
            max_length=MAX_LEN,
        ).to(DEVICE)
        with torch.no_grad():
            logits = self.model(**inputs).logits
        probs = F.softmax(logits, dim=-1).squeeze().tolist()
        pred_idx = int(np.argmax(probs))
        return {
            "label":      self.label_map[pred_idx],
            "confidence": round(probs[pred_idx], 4),
            "scores":     {self.label_map[i]: round(p, 4) for i, p in enumerate(probs)},
            "flagged":    pred_idx != 0,
        }


if __name__ == "__main__":
    import sys
    model_path = sys.argv[1] if len(sys.argv) > 1 else "./spam_injection_model/final"
    guard = GuardClassifier(model_path)
    while True:
        text = input("Input (blank to quit): ").strip()
        if not text:
            break
        r = guard.classify(text)
        print(f"  label={r['label']:20s}  confidence={r['confidence']:.4f}  flagged={r['flagged']}")