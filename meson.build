project('IdpProtocol', 'cpp')

srcs = []
inc = include_directories('src/cpp/')

srcs += 'src/cpp/DataReceivedEventArgs.cpp'
srcs += 'src/cpp/IdpCommandManager.cpp'
srcs += 'src/cpp/IdpNode.cpp'
srcs += 'src/cpp/IdpPacket.cpp'
srcs += 'src/cpp/IdpPacketParser.cpp'
srcs += 'src/cpp/IdpResponse.cpp'
srcs += 'src/cpp/IdpRouter.cpp'
srcs += 'src/cpp/IdpServerNode.cpp'
srcs += 'src/cpp/IncomingTransaction.cpp'
srcs += 'src/cpp/MasterNode.cpp'
srcs += 'src/cpp/NotifyingStreamAdaptor.cpp'
srcs += 'src/cpp/OutgoingTransaction.cpp'
srcs += 'src/cpp/SimpleAdaptor.cpp'

avalonapplication_proj = subproject('AvalonApplication')
avalonapplication_dep = avalonapplication_proj.get_variable('avalonapplication_dep')

commonhal_proj = subproject('CommonHal')
commonhal_dep = commonhal_proj.get_variable('commonhal_dep')

dispatcher_proj = subproject('Dispatcher')
dispatcher_dep = dispatcher_proj.get_variable('dispatcher_dep')

utils_proj = subproject('Utils')
utils_dep = utils_proj.get_variable('utils_dep')

libidpprotocol = static_library('idpprotocol',
    srcs,
    include_directories: inc,
    dependencies: [avalonapplication_dep, commonhal_dep, dispatcher_dep, utils_dep])

idpprotocol_dep = declare_dependency(include_directories: inc, link_with: libidpprotocol)

