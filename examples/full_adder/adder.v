module adder (cin, a1, a2, sum, cout);

input   wire    cin;
input   wire    a1;
input   wire    a2;

output  wire    sum;
output  wire    cout;

assign {cout, sum} = a1 + a2 + cin;

endmodule
