---
source_url: file:///mnt/CharlieII/Permissions.txt
ingested: 2026-05-03
sha256: d46c110050dc9b356401adcd94e9ceea7aa5af2803997badeb1033cdf1016e0d
---

The core concept: Linux ownership
Every file and directory in Linux has three pieces of identity:
Owner (user)  |  Group  |  Everyone else
And three permission types for each: read, write, execute.

chown — who owns it
change owner. It answers: "Which user/group is responsible for this file?"
bash chown 1000:1000 /opt/data
#      ^uid  ^gid

The first number is the user ID (UID)
The second is the group ID (GID)
Linux doesn't care about names — only numbers underneath

bash chown alice:developers /opt/data   # by name
chown 1000:1000 /opt/data          # by ID (same thing if alice=1000)

chmod — what can be done with it
change mode. It answers: "What actions are allowed, and for whom?"
bashchmod 777 /opt/data
#     ^^^
#     │││
#     ││└── everyone else: 7 = rwx (read+write+execute)
#     │└─── group:         7 = rwx
#     └──── owner:         7 = rwx
The numbers are binary flags added together:
NumberMeaning4read2write1execute7read + write + execute6read + write5read + execute
So chmod 755 means: owner can do everything (7), group and others can only read/execute (5).
