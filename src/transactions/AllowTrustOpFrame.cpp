// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/AllowTrustOpFrame.h"
#include "ledger/LedgerManager.h"
#include "ledger/TrustFrame.h"
#include "database/Database.h"

namespace stellar
{
AllowTrustOpFrame::AllowTrustOpFrame(Operation const& op, OperationResult& res,
                                     TransactionFrame& parentTx)
    : OperationFrame(op, res, parentTx)
    , mAllowTrust(mOperation.body.allowTrustOp())
{
}

int32_t
AllowTrustOpFrame::getNeededThreshold() const
{
    return mSourceAccount->getLowThreshold();
}

bool
AllowTrustOpFrame::doApply(LedgerDelta& delta, LedgerManager& ledgerManager)
{
    if (!(mSourceAccount->getAccount().flags & AUTH_REQUIRED_FLAG))
    { // this account doesn't require authorization to hold credit
        innerResult().code(ALLOW_TRUST_TRUST_NOT_REQUIRED);
        return false;
    }

    if (!(mSourceAccount->getAccount().flags & AUTH_REVOCABLE_FLAG) &&
        !mAllowTrust.authorize)
    {
        innerResult().code(ALLOW_TRUST_CANT_REVOKE);
        return false;
    }

    Currency ci;
    ci.type(CURRENCY_TYPE_ALPHANUM);
    ci.alphaNum().currencyCode = mAllowTrust.currency.currencyCode();
    ci.alphaNum().issuer = getSourceID();

    Database& db = ledgerManager.getDatabase();
    TrustFrame::pointer trustLine;
    trustLine = TrustFrame::loadTrustLine(mAllowTrust.trustor, ci, db);

    if (!trustLine)
    {
        innerResult().code(ALLOW_TRUST_NO_TRUST_LINE);
        return false;
    }

    innerResult().code(ALLOW_TRUST_SUCCESS);

    trustLine->setAuthorized(mAllowTrust.authorize);

    trustLine->storeChange(delta, db);

    return true;
}

bool
AllowTrustOpFrame::doCheckValid()
{
    if (mAllowTrust.currency.type() != CURRENCY_TYPE_ALPHANUM)
    {
        innerResult().code(ALLOW_TRUST_MALFORMED);
        return false;
    }
    Currency ci;
    ci.type(CURRENCY_TYPE_ALPHANUM);
    ci.alphaNum().currencyCode = mAllowTrust.currency.currencyCode();
    ci.alphaNum().issuer = getSourceID();

    if (!isCurrencyValid(ci))
    {
        innerResult().code(ALLOW_TRUST_MALFORMED);
        return false;
    }

    return true;
}
}
