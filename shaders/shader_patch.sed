s/ArrayStride 16/ArrayStride 4/g
# wst offset:
s/OpMemberDecorate %Private_UXN_Buffer 1 Offset 1048576/OpMemberDecorate %Private_UXN_Buffer 1 Offset 262144/g
# wst pointer:
s/OpMemberDecorate %Private_UXN_Buffer 2 Offset 1052672/OpMemberDecorate %Private_UXN_Buffer 2 Offset 263168/g
# rst:
s/OpMemberDecorate %Private_UXN_Buffer 3 Offset 1052688/OpMemberDecorate %Private_UXN_Buffer 3 Offset 263172/g
# rst pointer:
s/OpMemberDecorate %Private_UXN_Buffer 4 Offset 1056784/OpMemberDecorate %Private_UXN_Buffer 4 Offset 264196/g

# dev:
s/OpMemberDecorate %Shared_UXN_Buffer 1 Offset 16/OpMemberDecorate %Shared_UXN_Buffer 1 Offset 4/g
# flag:
s/OpMemberDecorate %Shared_UXN_Buffer 2 Offset 4112/OpMemberDecorate %Shared_UXN_Buffer 2 Offset 1028/g
# halt:
s/OpMemberDecorate %Shared_UXN_Buffer 3 Offset 4116/OpMemberDecorate %Shared_UXN_Buffer 3 Offset 1032/g

