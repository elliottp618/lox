namespace Lox.Syntax {

interface Visitor<R> {
    // expressions
    R VisitBinaryExpression( BinaryExpression e );
    R VisitGroupExpression( GroupExpression e );
    R VisitLiteralExpression( LiteralExpression e );
    R VisitLogicalExpression( LogicalExpression e );
    R VisitCallExpression( CallExpression e );
    R VisitUnaryExpression( UnaryExpression e );
    R VisitVariableExpression( VariableExpression e );
    R VisitAssignmentExpression( AssignmentExpression e );

    // statements
    R VisitExpressionStatement( ExpressionStatement s );
    R VisitBlockStatement( BlockStatement s );
    R VisitVariableDeclarationStatement( VariableDeclarationStatement s );
    R VisitIfStatement( IfStatement s );
    R VisitWhileStatement( WhileStatement s );
    R VisitFunctionDeclarationStatement( FunctionDeclarationStatement s );
    R VisitReturnStatement( ReturnStatement s );
}

} // namespace
