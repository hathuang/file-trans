echo "Linux, 64bits"
make arch=linux bits=64 target=server
#echo "Windows, 64bits"
#make arch=win bits=64
#echo "Linux, 32bits"
#make arch=linux bits=32
echo "Windows, 32bits"
make arch=win bits=32 target=client
make arch=win bits=32 target=server
echo ""
echo ""
echo "--- Enjoy it ! ---"
echo ""
