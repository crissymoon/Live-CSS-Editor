// emoji.js -- emoji popup and insertion

const EMOJIS = [
    '🫦','🏳️‍⚧️','🏳️‍🌈','—','←','→','©',
    '📝','✍️','✅',
    '😂','❤️','🤣','👍','😭','🙏','😘','🥰','😍','😊',
    '🎉','🤔','😁','🔥','💕','😎','✨','😒','👏','😢',
    '😜','💯','👀','🙄','🤷','😳','🤦','🙌','🤢','😴',
    '😅','🤩','😡','🤮','🤪','😃','👎','😏','🤗','🤯',
    '😱','😤','😋','🥺','😷','🤧','😇','🥳','🤠','😈',
    '💩','👻','🤘','💪','👊','✌️','🤝','🙈','🙉','🙊',
    '💖','💔','💘','💝','💓','💗','💞','❣️','💟','🧡',
    '💛','💚','💙','💜','🖤','🤎','🤍','😺','😸','😹',
    '😻','😼','😽','🙀','😿','😾','👶','🧒','👧','🧑',
    '👨','🧔','👩','🧓','👴','👵'
];

document.addEventListener('DOMContentLoaded', function() {
    const container = document.getElementById('emoji-container');
    const editor    = document.getElementById('editor');

    // Build emoji buttons
    EMOJIS.forEach(emoji => {
        const btn = document.createElement('button');
        btn.className = 'emoji-btn';
        btn.textContent = emoji;
        btn.addEventListener('click', function() {
            editor.focus();
            const sel = window.getSelection();
            if (sel.rangeCount > 0) {
                const range = sel.getRangeAt(0);
                range.deleteContents();
                const node = document.createTextNode(emoji);
                range.insertNode(node);
                range.setStartAfter(node);
                range.setEndAfter(node);
                range.collapse(false);
                sel.removeAllRanges();
                sel.addRange(range);
            } else {
                editor.appendChild(document.createTextNode(emoji));
            }
            setTimeout(saveState, 100);
        });
        container.appendChild(btn);
    });

    // Open / close popup
    document.getElementById('openPopup').addEventListener('click', function() {
        document.getElementById('emojiPopup').style.display = 'block';
    });
    document.getElementById('closePopup').addEventListener('click', function() {
        document.getElementById('emojiPopup').style.display = 'none';
    });
    window.addEventListener('click', function(e) {
        if (e.target === document.getElementById('emojiPopup'))
            document.getElementById('emojiPopup').style.display = 'none';
    });
});
