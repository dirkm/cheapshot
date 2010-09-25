def dump_type(repr):
  for(int i=7;i>=0;--i)
    for(int j=i*8;j<(i+1)*8;++j)
    std::cout<<repr[j];
    std::cout << "\n";
